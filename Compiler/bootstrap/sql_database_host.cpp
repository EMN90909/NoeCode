#include "noe.hpp"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
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
std::optional<std::string> takeId(Cursor&c,Diagnostics&d,const char*what){if(c.end()||!identifier(c.peek())){d.error("NQR-D8103",{},std::string("expected ")+what);return std::nullopt;}return c.take();}
bool literalToken(const std::string&s){if(s.empty())return false;if(s.front()=='\"'&&s.back()=='\"')return true;if(ieq(s,"true")||ieq(s,"false"))return true;std::size_t p=0;if(s[0]=='-')p=1;bool digit=false,dot=false;for(;p<s.size();++p){if(std::isdigit(static_cast<unsigned char>(s[p]))){digit=true;continue;}if(s[p]=='.'&&!dot){dot=true;continue;}return false;}return digit;}
std::optional<std::string> takeLiteral(Cursor&c,Diagnostics&d){if(c.end()||!literalToken(c.peek())){d.error("NQR-D8104",{},"expected SQL literal");return std::nullopt;}return c.take();}
std::string mapType(const std::string&type){auto u=upper(type);if(u=="INT"||u=="INTEGER"||u=="BIGINT")return"int";if(u=="REAL"||u=="FLOAT"||u=="DOUBLE"||u=="NUMERIC")return"real";if(u=="BOOL"||u=="BOOLEAN")return"bool";if(u=="TEXT"||u=="STRING"||u=="VARCHAR"||u=="CHAR")return"text";return{};}

struct SelectPlan {std::vector<std::string> columns;std::string table;std::optional<std::pair<std::string,std::string>> where;std::string orderBy;bool desc=false;};

std::optional<std::string> translate(Cursor&c,Diagnostics&d,std::optional<SelectPlan>&select){
    if(c.match("BEGIN")||c.match("COMMIT")){if(!c.end()&&!c.match("TRANSACTION")){d.error("NQR-D8105",{},"unexpected tokens after transaction keyword");return std::nullopt;}return std::string{};}
    if(c.match("ROLLBACK")){return std::string("__ROLLBACK__");}
    if(c.match("CREATE")){
        if(!c.match("TABLE")){d.error("NQR-D8106",{},"only CREATE TABLE is supported");return std::nullopt;}auto table=takeId(c,d,"table name");if(!table||!c.symbol("("))return std::nullopt;std::ostringstream nqd;nqd<<"table "<<*table<<" { ";bool first=true;
        while(!c.end()&&!c.symbol(")")){auto name=takeId(c,d,"column name");if(!name)return std::nullopt;auto rawType=takeId(c,d,"column type");if(!rawType)return std::nullopt;auto type=mapType(*rawType);if(type.empty()){d.error("NQR-D8107",{},"unsupported SQL column type '"+*rawType+"'");return std::nullopt;}bool required=false,unique=false,key=false;while(!c.end()&&c.peek()!=","&&c.peek()!=")"){if(c.match("PRIMARY")){if(!c.match("KEY")){d.error("NQR-D8108",{},"PRIMARY must be followed by KEY");return std::nullopt;}key=unique=required=true;}else if(c.match("NOT")){if(!c.match("NULL")){d.error("NQR-D8109",{},"NOT must be followed by NULL");return std::nullopt;}required=true;}else if(c.match("UNIQUE"))unique=true;else{d.error("NQR-D8110",{},"unsupported CREATE TABLE column constraint '"+c.peek()+"'");return std::nullopt;}}
            if(!first)nqd<<", ";first=false;nqd<<*name<<": "<<type;if(required)nqd<<" required";if(unique)nqd<<" unique";if(key)nqd<<" key";c.symbol(",");}
        if(!c.end()){d.error("NQR-D8111",{},"unexpected tokens after CREATE TABLE");return std::nullopt;}nqd<<" }";return nqd.str();
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
        SelectPlan plan;if(c.symbol("*"))plan.columns.push_back("*");else{do{auto col=takeId(c,d,"selected column");if(!col)return std::nullopt;plan.columns.push_back(*col);}while(c.symbol(","));}if(!c.match("FROM")){d.error("NQR-D8117",{},"SELECT requires FROM");return std::nullopt;}auto table=takeId(c,d,"table name");if(!table)return std::nullopt;plan.table=*table;if(c.match("WHERE")){auto col=takeId(c,d,"WHERE column");if(!col||!c.symbol("="))return std::nullopt;auto value=takeLiteral(c,d);if(!value)return std::nullopt;plan.where=std::make_pair(*col,*value);}if(c.match("ORDER")){if(!c.match("BY")){d.error("NQR-D8118",{},"ORDER must be followed by BY");return std::nullopt;}auto col=takeId(c,d,"ORDER BY column");if(!col)return std::nullopt;plan.orderBy=*col;if(c.match("DESC"))plan.desc=true;else c.match("ASC");}if(!c.end()){d.error("NQR-D8119",{},"JOIN, GROUP BY and other SELECT clauses are not yet supported by the bootstrap SQL adapter");return std::nullopt;}select=plan;std::ostringstream nqd;nqd<<"select "<<plan.table;if(plan.where)nqd<<" where "<<plan.where->first<<" = "<<plan.where->second;return nqd.str();
    }
    d.error("NQR-D8120",{},"unsupported SQL statement; expected CREATE TABLE, INSERT, SELECT, UPDATE, DELETE, BEGIN, COMMIT or ROLLBACK");return std::nullopt;
}

std::vector<std::string> splitTabs(const std::string&line){std::vector<std::string>out;std::istringstream in(line);std::string part;while(std::getline(in,part,'\t'))out.push_back(part);return out;}
bool emitSelect(const SelectPlan&plan,const std::string&tsv,std::ostream&out,Diagnostics&d){std::istringstream input(tsv);std::string line;if(!std::getline(input,line)){d.error("NQR-D8121",{},"SELECT returned no schema row");return false;}auto headers=splitTabs(line);std::vector<std::size_t>projection;if(plan.columns.size()==1&&plan.columns[0]=="*"){for(std::size_t i=0;i<headers.size();++i)projection.push_back(i);}else for(const auto&name:plan.columns){auto it=std::find(headers.begin(),headers.end(),name);if(it==headers.end()){d.error("NQR-D8122",{},"unknown selected column '"+name+"'");return false;}projection.push_back(static_cast<std::size_t>(it-headers.begin()));}std::optional<std::size_t>order;if(!plan.orderBy.empty()){auto it=std::find(headers.begin(),headers.end(),plan.orderBy);if(it==headers.end()){d.error("NQR-D8123",{},"unknown ORDER BY column '"+plan.orderBy+"'");return false;}order=static_cast<std::size_t>(it-headers.begin());}std::vector<std::vector<std::string>>rows;while(std::getline(input,line))rows.push_back(splitTabs(line));if(order)std::stable_sort(rows.begin(),rows.end(),[&](const auto&a,const auto&b){const std::string av=*order<a.size()?a[*order]:"",bv=*order<b.size()?b[*order]:"";double an=0,bn=0;bool na=false,nb=false;try{std::size_t p=0;an=std::stod(av,&p);na=p==av.size();}catch(...){ }try{std::size_t p=0;bn=std::stod(bv,&p);nb=p==bv.size();}catch(...){ }bool less=na&&nb?an<bn:av<bv;return plan.desc?!less&&av!=bv:less;});for(std::size_t i=0;i<projection.size();++i){if(i)out<<'\t';out<<headers[projection[i]];}out<<'\n';for(const auto&r:rows){for(std::size_t i=0;i<projection.size();++i){if(i)out<<'\t';auto idx=projection[i];if(idx<r.size())out<<r[idx];}out<<'\n';}return true;}
}

bool NoqeriDatabase::executeSql(const std::filesystem::path&script,const std::filesystem::path&databaseOverride,std::ostream&output,Diagnostics&diagnostics)const{
    std::ifstream in(script,std::ios::binary);if(!in){diagnostics.error("NQR-D8124",{},"cannot open .sql script: "+script.string());return false;}std::ostringstream buffer;buffer<<in.rdbuf();auto statements=splitStatements(buffer.str(),diagnostics);if(diagnostics.hasErrors())return false;
    auto database=databaseOverride;if(database.empty()){database=script;database.replace_extension(".nqdb");}
    const auto stamp=std::chrono::steady_clock::now().time_since_epoch().count();auto shadow=database;shadow+=".sqltx-"+std::to_string(stamp);std::error_code ec;if(std::filesystem::exists(database)){std::filesystem::copy_file(database,shadow,std::filesystem::copy_options::overwrite_existing,ec);if(ec){diagnostics.error("NQR-D8125",{},"cannot create SQL transaction shadow: "+ec.message());return false;}}
    bool rollback=false;std::size_t index=0;
    for(const auto&statement:statements){Diagnostics parseDiagnostics;auto tokens=tokenize(statement,parseDiagnostics);if(parseDiagnostics.hasErrors()){diagnostics=parseDiagnostics;rollback=true;break;}Cursor cursor{std::move(tokens)};std::optional<SelectPlan>select;auto nqd=translate(cursor,parseDiagnostics,select);if(!nqd){diagnostics=parseDiagnostics;rollback=true;break;}if(*nqd=="__ROLLBACK__"){rollback=true;break;}if(nqd->empty())continue;auto temp=script.parent_path()/(".noqeri-sql-"+std::to_string(stamp)+"-"+std::to_string(index++)+".nqd");{std::ofstream out(temp,std::ios::trunc);out<<*nqd<<"\n";}Diagnostics runDiagnostics;std::ostringstream captured;if(!execute(temp,shadow,captured,runDiagnostics)){diagnostics=runDiagnostics;std::filesystem::remove(temp,ec);rollback=true;break;}std::filesystem::remove(temp,ec);if(select&&!emitSelect(*select,captured.str(),output,diagnostics)){rollback=true;break;}}
    if(rollback){std::filesystem::remove(shadow,ec);if(!diagnostics.hasErrors())diagnostics.error("NQR-D8126",{},"SQL transaction rolled back");return false;}
    if(std::filesystem::exists(shadow)){auto parent=database.parent_path();if(!parent.empty())std::filesystem::create_directories(parent,ec);ec.clear();std::filesystem::remove(database,ec);ec.clear();std::filesystem::rename(shadow,database,ec);if(ec){diagnostics.error("NQR-D8127",{},"cannot commit SQL transaction: "+ec.message());std::filesystem::remove(shadow,ec);return false;}}
    return true;
}

} // namespace noe
