#include "noe.hpp"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace noe {
namespace {
std::string trim(std::string s){auto non=[](unsigned char c){return !std::isspace(c);};s.erase(s.begin(),std::find_if(s.begin(),s.end(),non));s.erase(std::find_if(s.rbegin(),s.rend(),non).base(),s.end());return s;}
std::string upper(std::string s){for(char&c:s)c=static_cast<char>(std::toupper(static_cast<unsigned char>(c)));return s;}
bool ieq(const std::string&a,const std::string&b){return upper(a)==upper(b);}

std::vector<std::string> splitStatements(const std::string&source,Diagnostics&d){
    std::vector<std::string> out;std::string current;char quote=0;bool escape=false;bool lineComment=false,blockComment=false;
    for(std::size_t i=0;i<source.size();++i){char c=source[i],n=i+1<source.size()?source[i+1]:'\0';
        if(lineComment){if(c=='\n')lineComment=false;continue;}
        if(blockComment){if(c=='*'&&n=='/'){blockComment=false;++i;}continue;}
        if(!quote&&c=='-'&&n=='-'){lineComment=true;++i;continue;}
        if(!quote&&c=='/'&&n=='*'){blockComment=true;++i;continue;}
        if(quote){current+=c;if(escape){escape=false;continue;}if(c=='\\'){escape=true;continue;}if(c==quote){if(i+1<source.size()&&source[i+1]==quote){current+=source[++i];continue;}quote=0;}continue;}
        if(c=='\''||c=='\"'){quote=c;current+=c;continue;}
        if(c==';'){auto statement=trim(current);if(!statement.empty())out.push_back(statement);current.clear();continue;}
        current+=c;
    }
    if(quote||blockComment){d.error("NQR-D8100",{},"unterminated SQL string or block comment");return {};}
    auto last=trim(current);if(!last.empty())out.push_back(last);return out;
}

std::vector<std::string> tokenize(const std::string&s,Diagnostics&d){
    std::vector<std::string> out;std::size_t i=0;
    while(i<s.size()){
        if(std::isspace(static_cast<unsigned char>(s[i]))){++i;continue;}
        char c=s[i];
        if(c=='\''||c=='\"'){
            const char quote=c;std::string value;bool closed=false;++i;
            while(i<s.size()){char x=s[i++];if(x==quote){if(i<s.size()&&s[i]==quote){value+=quote;++i;continue;}closed=true;break;}if(x=='\\'&&i<s.size()){char e=s[i++];if(e=='n')value+='\n';else if(e=='r')value+='\r';else if(e=='t')value+='\t';else value+=e;}else value+=x;}
            if(!closed){d.error("NQR-D8101",{},"unterminated SQL string");return {};}
            std::string quoted="\"";for(char x:value){if(x=='\\'||x=='\"')quoted+='\\';quoted+=x;}quoted+='\"';out.push_back(std::move(quoted));continue;
        }
        if(std::isalnum(static_cast<unsigned char>(c))||c=='_'||c=='-'||c=='.'){
            std::string v;while(i<s.size()){char x=s[i];if(!std::isalnum(static_cast<unsigned char>(x))&&x!='_'&&x!='-'&&x!='.')break;v+=x;++i;}out.push_back(std::move(v));continue;
        }
        if(std::string("(),=*?").find(c)!=std::string::npos){out.push_back(std::string(1,c));++i;continue;}
        d.error("NQR-D8102",{},std::string("unsupported SQL character '")+c+"'");return {};
    }
    return out;
}

struct Cursor {std::vector<std::string> t;std::size_t i=0;bool end()const{return i>=t.size();}std::string peek()const{return end()?"":t[i];}std::string take(){return end()?"":t[i++];}bool match(const std::string&v){if(!end()&&ieq(t[i],v)){++i;return true;}return false;}bool symbol(const std::string&v){if(!end()&&t[i]==v){++i;return true;}return false;}};
bool identifier(const std::string&s){if(s.empty()||!(std::isalpha(static_cast<unsigned char>(s[0]))||s[0]=='_'))return false;return std::all_of(s.begin()+1,s.end(),[](unsigned char c){return std::isalnum(c)||c=='_';});}
bool sqlName(const std::string&s){auto dot=s.find('.');if(dot==std::string::npos)return identifier(s);return dot>0&&dot+1<s.size()&&s.find('.',dot+1)==std::string::npos&&identifier(s.substr(0,dot))&&identifier(s.substr(dot+1));}
std::optional<std::string> takeId(Cursor&c,Diagnostics&d,const char*what){if(c.end()||!identifier(c.peek())){d.error("NQR-D8103",{},std::string("expected ")+what);return std::nullopt;}return c.take();}
std::optional<std::string> takeName(Cursor&c,Diagnostics&d,const char*what){if(c.end()||!sqlName(c.peek())){d.error("NQR-D8103",{},std::string("expected ")+what);return std::nullopt;}return c.take();}
bool literalToken(const std::string&s){if(s.empty())return false;if(s.size()>=2&&s.front()=='\"'&&s.back()=='\"')return true;if(ieq(s,"true")||ieq(s,"false")||ieq(s,"null"))return true;std::size_t p=0;if(s[0]=='-')p=1;bool digit=false,dot=false;for(;p<s.size();++p){if(std::isdigit(static_cast<unsigned char>(s[p]))){digit=true;continue;}if(s[p]=='.'&&!dot){dot=true;continue;}return false;}return digit;}
std::optional<std::string> takeLiteral(Cursor&c,Diagnostics&d){if(c.end()||!literalToken(c.peek())){d.error("NQR-D8104",{},"expected SQL literal");return std::nullopt;}return c.take();}
std::string literalValue(const std::string&s){if(s.size()>=2&&s.front()=='\"'&&s.back()=='\"'){std::string out;for(std::size_t i=1;i+1<s.size();++i){if(s[i]=='\\'&&i+2<s.size())out+=s[++i];else out+=s[i];}return out;}return ieq(s,"NULL")?"":s;}
std::string mapType(const std::string&type){auto u=upper(type);if(u=="INT"||u=="INTEGER"||u=="BIGINT")return"int";if(u=="REAL"||u=="FLOAT"||u=="DOUBLE"||u=="NUMERIC")return"real";if(u=="BOOL"||u=="BOOLEAN")return"bool";if(u=="TEXT"||u=="STRING"||u=="VARCHAR"||u=="CHAR")return"text";return{};}

enum class ExprKind { Column, Count, Sum, Min, Max };
struct SelectExpr { ExprKind kind=ExprKind::Column;std::string name;std::string alias; };
struct JoinPlan {std::string table,left,right;};
struct SelectPlan {
    bool star=false;std::vector<SelectExpr> expressions;std::string table;std::vector<JoinPlan> joins;
    std::optional<std::pair<std::string,std::string>> where;std::vector<std::string> groupBy;
    std::string orderBy;bool desc=false;
};

std::optional<SelectExpr> parseSelectExpr(Cursor&c,Diagnostics&d){
    if(c.end())return std::nullopt;SelectExpr expr;auto u=upper(c.peek());
    if(u=="COUNT"||u=="SUM"||u=="MIN"||u=="MAX"){
        c.take();expr.kind=u=="COUNT"?ExprKind::Count:u=="SUM"?ExprKind::Sum:u=="MIN"?ExprKind::Min:ExprKind::Max;
        if(!c.symbol("(")){d.error("NQR-D8128",{},u+" requires '('");return std::nullopt;}
        if(expr.kind==ExprKind::Count&&c.symbol("*"))expr.name="*";else{auto name=takeName(c,d,"aggregate column");if(!name)return std::nullopt;expr.name=*name;}
        if(!c.symbol(")")){d.error("NQR-D8129",{},u+" requires ')'");return std::nullopt;}
    }else{auto name=takeName(c,d,"selected column");if(!name)return std::nullopt;expr.name=*name;}
    if(c.match("AS")){auto alias=takeId(c,d,"column alias");if(!alias)return std::nullopt;expr.alias=*alias;}
    return expr;
}

std::optional<std::string> translate(Cursor&c,Diagnostics&d,std::optional<SelectPlan>&select){
    if(c.match("BEGIN")||c.match("COMMIT")){if(!c.end()&&!c.match("TRANSACTION")){d.error("NQR-D8105",{},"unexpected tokens after transaction keyword");return std::nullopt;}if(!c.end()){d.error("NQR-D8105",{},"unexpected tokens after transaction keyword");return std::nullopt;}return std::string{};}
    if(c.match("ROLLBACK")){if(!c.end()){d.error("NQR-D8105",{},"unexpected tokens after ROLLBACK");return std::nullopt;}return std::string("__ROLLBACK__");}
    if(c.match("CREATE")){
        if(!c.match("TABLE")){d.error("NQR-D8106",{},"only CREATE TABLE is supported");return std::nullopt;}auto table=takeId(c,d,"table name");if(!table||!c.symbol("("))return std::nullopt;std::ostringstream nqd;nqd<<"table "<<*table<<" { ";bool first=true,closed=false;
        while(!c.end()){if(c.symbol(")")){closed=true;break;}auto name=takeId(c,d,"column name");if(!name)return std::nullopt;auto rawType=takeId(c,d,"column type");if(!rawType)return std::nullopt;auto type=mapType(*rawType);if(type.empty()){d.error("NQR-D8107",{},"unsupported SQL column type '"+*rawType+"'");return std::nullopt;}bool required=false,unique=false,key=false;while(!c.end()&&c.peek()!=","&&c.peek()!=")"){if(c.match("PRIMARY")){if(!c.match("KEY")){d.error("NQR-D8108",{},"PRIMARY must be followed by KEY");return std::nullopt;}key=unique=required=true;}else if(c.match("NOT")){if(!c.match("NULL")){d.error("NQR-D8109",{},"NOT must be followed by NULL");return std::nullopt;}required=true;}else if(c.match("UNIQUE"))unique=true;else{d.error("NQR-D8110",{},"unsupported CREATE TABLE column constraint '"+c.peek()+"'");return std::nullopt;}}
            if(!first)nqd<<", ";first=false;nqd<<*name<<": "<<type;if(required)nqd<<" required";if(unique)nqd<<" unique";if(key)nqd<<" key";if(c.symbol(","))continue;if(c.peek()!=")"){d.error("NQR-D8111",{},"expected ',' or ')' in CREATE TABLE");return std::nullopt;}}
        if(!closed){d.error("NQR-D8111",{},"CREATE TABLE is missing closing ')'");return std::nullopt;}if(!c.end()){d.error("NQR-D8111",{},"unexpected tokens after CREATE TABLE");return std::nullopt;}nqd<<" }";return nqd.str();
    }
    if(c.match("INSERT")){
        if(!c.match("INTO")){d.error("NQR-D8112",{},"INSERT must use INTO");return std::nullopt;}auto table=takeId(c,d,"table name");if(!table||!c.symbol("("))return std::nullopt;std::vector<std::string>cols;do{auto col=takeId(c,d,"column name");if(!col)return std::nullopt;cols.push_back(*col);}while(c.symbol(","));if(!c.symbol(")")||!c.match("VALUES")||!c.symbol("("))return std::nullopt;std::vector<std::string>vals;do{auto v=takeLiteral(c,d);if(!v)return std::nullopt;vals.push_back(*v);}while(c.symbol(","));if(!c.symbol(")")||cols.size()!=vals.size()||!c.end()){d.error("NQR-D8113",{},"INSERT column/value count mismatch or trailing tokens");return std::nullopt;}std::ostringstream nqd;nqd<<"insert "<<*table<<" { ";for(std::size_t i=0;i<cols.size();++i){if(i)nqd<<", ";nqd<<cols[i]<<": "<<vals[i];}nqd<<" }";return nqd.str();
    }
    if(c.match("UPDATE")){
        auto table=takeId(c,d,"table name");if(!table||!c.match("SET"))return std::nullopt;std::vector<std::pair<std::string,std::string>>set;for(;;){auto col=takeId(c,d,"column name");if(!col||!c.symbol("="))return std::nullopt;auto value=takeLiteral(c,d);if(!value)return std::nullopt;set.push_back({*col,*value});if(!c.symbol(","))break;}std::optional<std::pair<std::string,std::string>>where;if(c.match("WHERE")){auto col=takeId(c,d,"WHERE column");if(!col||!c.symbol("="))return std::nullopt;auto value=takeLiteral(c,d);if(!value)return std::nullopt;where=std::make_pair(*col,*value);}if(!c.end()){d.error("NQR-D8114",{},"unsupported UPDATE tail");return std::nullopt;}std::ostringstream nqd;nqd<<"update "<<*table<<" set { ";for(std::size_t i=0;i<set.size();++i){if(i)nqd<<", ";nqd<<set[i].first<<": "<<set[i].second;}nqd<<" }";if(where)nqd<<" where "<<where->first<<" = "<<where->second;return nqd.str();
    }
    if(c.match("DELETE")){
        if(!c.match("FROM")){d.error("NQR-D8115",{},"DELETE must use FROM");return std::nullopt;}auto table=takeId(c,d,"table name");if(!table)return std::nullopt;std::ostringstream nqd;nqd<<"delete "<<*table;if(c.match("WHERE")){auto col=takeId(c,d,"WHERE column");if(!col||!c.symbol("="))return std::nullopt;auto value=takeLiteral(c,d);if(!value)return std::nullopt;nqd<<" where "<<*col<<" = "<<*value;}if(!c.end()){d.error("NQR-D8116",{},"unsupported DELETE tail");return std::nullopt;}return nqd.str();
    }
    if(c.match("SELECT")){
        SelectPlan plan;if(c.symbol("*"))plan.star=true;else{for(;;){auto expr=parseSelectExpr(c,d);if(!expr)return std::nullopt;plan.expressions.push_back(*expr);if(!c.symbol(","))break;}}
        if(!c.match("FROM")){d.error("NQR-D8117",{},"SELECT requires FROM");return std::nullopt;}auto table=takeId(c,d,"table name");if(!table)return std::nullopt;plan.table=*table;
        while(c.match("JOIN")||c.match("INNER")){if(ieq(c.t[c.i-1],"INNER")&&!c.match("JOIN")){d.error("NQR-D8130",{},"INNER must be followed by JOIN");return std::nullopt;}auto joined=takeId(c,d,"joined table");if(!joined||!c.match("ON"))return std::nullopt;auto left=takeName(c,d,"left join column");if(!left||!c.symbol("="))return std::nullopt;auto right=takeName(c,d,"right join column");if(!right)return std::nullopt;plan.joins.push_back({*joined,*left,*right});}
        if(c.match("WHERE")){auto col=takeName(c,d,"WHERE column");if(!col||!c.symbol("="))return std::nullopt;auto value=takeLiteral(c,d);if(!value)return std::nullopt;plan.where=std::make_pair(*col,*value);}
        if(c.match("GROUP")){if(!c.match("BY")){d.error("NQR-D8131",{},"GROUP must be followed by BY");return std::nullopt;}do{auto name=takeName(c,d,"GROUP BY column");if(!name)return std::nullopt;plan.groupBy.push_back(*name);}while(c.symbol(","));}
        if(c.match("ORDER")){if(!c.match("BY")){d.error("NQR-D8118",{},"ORDER must be followed by BY");return std::nullopt;}auto col=takeName(c,d,"ORDER BY column or alias");if(!col)return std::nullopt;plan.orderBy=*col;if(c.match("DESC"))plan.desc=true;else c.match("ASC");}
        if(!c.end()){d.error("NQR-D8119",{},"unsupported SELECT clause; bootstrap SQL supports inner equality joins, WHERE equality, GROUP BY and simple ORDER BY");return std::nullopt;}select=plan;return std::string("__SELECT__");
    }
    d.error("NQR-D8120",{},"unsupported SQL statement; expected CREATE TABLE, INSERT, SELECT, UPDATE, DELETE, BEGIN, COMMIT or ROLLBACK");return std::nullopt;
}

std::vector<std::string> splitTabs(const std::string&line){std::vector<std::string>out;std::istringstream in(line);std::string part;while(std::getline(in,part,'\t'))out.push_back(part);return out;}
struct Relation {std::vector<std::string> names;std::vector<std::vector<std::string>> rows;};
std::optional<std::size_t> resolveColumn(const std::vector<std::string>&names,const std::string&requested,Diagnostics&d){
    if(requested.find('.')!=std::string::npos){auto it=std::find(names.begin(),names.end(),requested);if(it==names.end()){d.error("NQR-D8132",{},"unknown column '"+requested+"'");return std::nullopt;}return static_cast<std::size_t>(it-names.begin());}
    std::optional<std::size_t> found;for(std::size_t i=0;i<names.size();++i){auto dot=names[i].find('.');auto simple=dot==std::string::npos?names[i]:names[i].substr(dot+1);if(simple==requested){if(found){d.error("NQR-D8133",{},"ambiguous column '"+requested+"'; qualify it with a table name");return std::nullopt;}found=i;}}
    if(!found)d.error("NQR-D8132",{},"unknown column '"+requested+"'");return found;
}
bool numeric(const std::string&s,double&value){try{std::size_t p=0;value=std::stod(s,&p);return p==s.size();}catch(...){return false;}}
int compareValue(const std::string&a,const std::string&b){double av=0,bv=0;const bool an=numeric(a,av),bn=numeric(b,bv);if(an&&bn){if(av<bv)return-1;if(av>bv)return 1;return a<b?-1:a>b?1:0;}return a<b?-1:a>b?1:0;}

bool loadRelation(const NoqeriDatabase&db,const std::filesystem::path&database,const std::filesystem::path&dir,long long stamp,std::size_t&counter,const std::string&table,Relation&relation,Diagnostics&d){
    const auto script=dir/(".noqeri-sql-read-"+std::to_string(stamp)+"-"+std::to_string(counter++)+".nqd");{std::ofstream out(script,std::ios::trunc);out<<"select "<<table<<"\n";}std::ostringstream captured;Diagnostics run;if(!db.execute(script,database,captured,run)){d=run;std::error_code ec;std::filesystem::remove(script,ec);return false;}std::error_code ec;std::filesystem::remove(script,ec);std::istringstream input(captured.str());std::string line;if(!std::getline(input,line)){d.error("NQR-D8134",{},"table '"+table+"' returned no schema row");return false;}auto headers=splitTabs(line);for(auto&h:headers)relation.names.push_back(table+"."+h);while(std::getline(input,line)){auto row=splitTabs(line);row.resize(headers.size());relation.rows.push_back(std::move(row));}return true;
}

bool executeSelect(const NoqeriDatabase&db,const SelectPlan&plan,const std::filesystem::path&database,const std::filesystem::path&dir,long long stamp,std::size_t&counter,std::ostream&out,Diagnostics&d){
    Relation current;if(!loadRelation(db,database,dir,stamp,counter,plan.table,current,d))return false;
    for(const auto&join:plan.joins){Relation right;if(!loadRelation(db,database,dir,stamp,counter,join.table,right,d))return false;std::vector<std::string>combinedNames=current.names;combinedNames.insert(combinedNames.end(),right.names.begin(),right.names.end());auto leftIndex=resolveColumn(combinedNames,join.left,d);if(!leftIndex)return false;auto rightIndex=resolveColumn(combinedNames,join.right,d);if(!rightIndex)return false;Relation joined;joined.names=combinedNames;for(const auto&leftRow:current.rows)for(const auto&rightRow:right.rows){std::vector<std::string>row=leftRow;row.insert(row.end(),rightRow.begin(),rightRow.end());if(*leftIndex<row.size()&&*rightIndex<row.size()&&row[*leftIndex]==row[*rightIndex])joined.rows.push_back(std::move(row));}current=std::move(joined);}
    if(plan.where){auto idx=resolveColumn(current.names,plan.where->first,d);if(!idx)return false;const auto expected=literalValue(plan.where->second);current.rows.erase(std::remove_if(current.rows.begin(),current.rows.end(),[&](const auto&r){return *idx>=r.size()||r[*idx]!=expected;}),current.rows.end());}

    struct ResolvedExpr {SelectExpr expr;std::optional<std::size_t> column;std::string header;};std::vector<ResolvedExpr>exprs;
    if(plan.star){for(std::size_t i=0;i<current.names.size();++i){SelectExpr e;e.name=current.names[i];exprs.push_back({e,i,current.names[i]});}}
    else for(const auto&e:plan.expressions){std::optional<std::size_t>col;if(e.name!="*"){col=resolveColumn(current.names,e.name,d);if(!col)return false;}std::string header=e.alias;if(header.empty()){const char*prefix=e.kind==ExprKind::Count?"COUNT":e.kind==ExprKind::Sum?"SUM":e.kind==ExprKind::Min?"MIN":e.kind==ExprKind::Max?"MAX":"";header=e.kind==ExprKind::Column?e.name:std::string(prefix)+"("+e.name+")";}exprs.push_back({e,col,header});}
    const bool aggregate=std::any_of(exprs.begin(),exprs.end(),[](const auto&e){return e.expr.kind!=ExprKind::Column;});
    std::vector<std::size_t>groupColumns;for(const auto&name:plan.groupBy){auto idx=resolveColumn(current.names,name,d);if(!idx)return false;groupColumns.push_back(*idx);}
    if(aggregate)for(const auto&e:exprs)if(e.expr.kind==ExprKind::Column&&std::find(groupColumns.begin(),groupColumns.end(),*e.column)==groupColumns.end()){d.error("NQR-D8135",{},"selected non-aggregate column '"+e.expr.name+"' must appear in GROUP BY");return false;}

    std::vector<std::vector<std::size_t>>groups;
    if(aggregate||!groupColumns.empty()){
        std::map<std::string,std::vector<std::size_t>>map;
        for(std::size_t r=0;r<current.rows.size();++r){std::string key;for(auto col:groupColumns){key+=col<current.rows[r].size()?current.rows[r][col]:"";key.push_back('\x1f');}map[key].push_back(r);}
        if(map.empty()&&aggregate&&groupColumns.empty())groups.push_back({});else for(auto&entry:map)groups.push_back(std::move(entry.second));
    }else for(std::size_t r=0;r<current.rows.size();++r)groups.push_back({r});

    std::vector<std::vector<std::string>>projected;
    for(const auto&group:groups){std::vector<std::string>row;for(const auto&e:exprs){
        if(e.expr.kind==ExprKind::Column){row.push_back(group.empty()?"":current.rows[group.front()][*e.column]);continue;}
        if(e.expr.kind==ExprKind::Count){if(e.expr.name=="*")row.push_back(std::to_string(group.size()));else{std::size_t count=0;for(auto r:group)if(*e.column<current.rows[r].size()&&!current.rows[r][*e.column].empty())++count;row.push_back(std::to_string(count));}continue;}
        if(e.expr.kind==ExprKind::Sum){double sum=0;bool allInt=true;for(auto r:group){if(*e.column>=current.rows[r].size())continue;double value=0;if(!numeric(current.rows[r][*e.column],value)){d.error("NQR-D8136",{},"SUM requires numeric values");return false;}sum+=value;if(current.rows[r][*e.column].find('.')!=std::string::npos)allInt=false;}std::ostringstream text;if(allInt)text<<static_cast<long long>(sum);else text<<sum;row.push_back(text.str());continue;}
        std::string best;bool set=false;for(auto r:group){if(*e.column>=current.rows[r].size())continue;const auto&value=current.rows[r][*e.column];if(!set||(e.expr.kind==ExprKind::Min?compareValue(value,best)<0:compareValue(value,best)>0)){best=value;set=true;}}row.push_back(best);
    }projected.push_back(std::move(row));}

    if(!plan.orderBy.empty()){
        std::optional<std::size_t>order;for(std::size_t i=0;i<exprs.size();++i)if(exprs[i].header==plan.orderBy||exprs[i].expr.name==plan.orderBy){if(order){d.error("NQR-D8137",{},"ambiguous ORDER BY column or alias '"+plan.orderBy+"'");return false;}order=i;}
        if(!order){d.error("NQR-D8138",{},"ORDER BY must reference a selected column or alias in the bootstrap adapter");return false;}
        std::stable_sort(projected.begin(),projected.end(),[&](const auto&a,const auto&b){const auto&av=a[*order],&bv=b[*order];const auto cmp=compareValue(av,bv);return plan.desc?cmp>0:cmp<0;});
    }
    for(std::size_t i=0;i<exprs.size();++i){if(i)out<<'\t';out<<exprs[i].header;}out<<'\n';for(const auto&r:projected){for(std::size_t i=0;i<r.size();++i){if(i)out<<'\t';out<<r[i];}out<<'\n';}return true;
}
}

bool NoqeriDatabase::executeSql(const std::filesystem::path&script,const std::filesystem::path&databaseOverride,std::ostream&output,Diagnostics&diagnostics)const{
    std::ifstream in(script,std::ios::binary);if(!in){diagnostics.error("NQR-D8124",{},"cannot open .sql script: "+script.string());return false;}std::ostringstream buffer;buffer<<in.rdbuf();auto statements=splitStatements(buffer.str(),diagnostics);if(diagnostics.hasErrors())return false;
    auto database=databaseOverride;if(database.empty()){database=script;database.replace_extension(".nqdb");}
    const auto stamp=std::chrono::steady_clock::now().time_since_epoch().count();auto shadow=database;shadow+=".sqltx-"+std::to_string(stamp);std::error_code ec;if(std::filesystem::exists(database)){std::filesystem::copy_file(database,shadow,std::filesystem::copy_options::overwrite_existing,ec);if(ec){diagnostics.error("NQR-D8125",{},"cannot create SQL transaction shadow: "+ec.message());return false;}}
    bool rollback=false,explicitRollback=false;std::size_t index=0;
    for(const auto&statement:statements){Diagnostics parseDiagnostics;auto tokens=tokenize(statement,parseDiagnostics);if(parseDiagnostics.hasErrors()){diagnostics=parseDiagnostics;rollback=true;break;}Cursor cursor{std::move(tokens)};std::optional<SelectPlan>select;auto nqd=translate(cursor,parseDiagnostics,select);if(!nqd){diagnostics=parseDiagnostics;rollback=true;break;}if(*nqd=="__ROLLBACK__"){rollback=explicitRollback=true;break;}if(*nqd=="__SELECT__"){if(!select||!executeSelect(*this,*select,shadow,database.parent_path().empty()?script.parent_path():database.parent_path(),stamp,index,output,diagnostics)){rollback=true;break;}continue;}if(nqd->empty())continue;auto temp=script.parent_path()/(".noqeri-sql-"+std::to_string(stamp)+"-"+std::to_string(index++)+".nqd");{std::ofstream out(temp,std::ios::trunc);out<<*nqd<<"\n";}Diagnostics runDiagnostics;std::ostringstream captured;if(!execute(temp,shadow,captured,runDiagnostics)){diagnostics=runDiagnostics;std::filesystem::remove(temp,ec);rollback=true;break;}std::filesystem::remove(temp,ec);}
    if(rollback){std::filesystem::remove(shadow,ec);if(explicitRollback&&!diagnostics.hasErrors())return true;if(!diagnostics.hasErrors())diagnostics.error("NQR-D8126",{},"SQL transaction rolled back");return false;}
    if(std::filesystem::exists(shadow)){auto parent=database.parent_path();if(!parent.empty())std::filesystem::create_directories(parent,ec);ec.clear();std::filesystem::remove(database,ec);ec.clear();std::filesystem::rename(shadow,database,ec);if(ec){diagnostics.error("NQR-D8127",{},"cannot commit SQL transaction: "+ec.message());std::filesystem::remove(shadow,ec);return false;}}
    return true;
}

} // namespace noe
