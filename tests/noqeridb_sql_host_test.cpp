#include "noqeri.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

int main(){
    using namespace noe;
    const auto root=std::filesystem::temp_directory_path()/("noqeri-sql-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(root);
    const auto script=root/"app.sql",database=root/"app.nqdb";
    {
        std::ofstream out(script);
        out<<"CREATE TABLE users (id INT PRIMARY KEY, name TEXT NOT NULL, active BOOL NOT NULL);\n"
           <<"INSERT INTO users (id,name,active) VALUES (2,'Linus',true);\n"
           <<"INSERT INTO users (id,name,active) VALUES (1,'Ada',true);\n"
           <<"UPDATE users SET name='Ada Lovelace' WHERE id=1;\n"
           <<"SELECT id,name FROM users WHERE active=true ORDER BY id ASC;\n";
    }
    Diagnostics d;std::ostringstream output;
    if(!NoqeriDatabase{}.executeSql(script,database,output,d)){d.print(script.string());std::filesystem::remove_all(root);return 1;}
    const auto text=output.str();
    if(text.find("id\tname") == std::string::npos || text.find("1\tAda Lovelace") == std::string::npos || text.find("2\tLinus") == std::string::npos){std::cerr<<text;std::filesystem::remove_all(root);return 2;}
    // A failing second script must not partially alter the durable database.
    const auto bad=root/"bad.sql";{std::ofstream out(bad);out<<"UPDATE users SET name='Broken' WHERE id=1; INSERT INTO missing (id) VALUES (3);\n";}
    Diagnostics badD;std::ostringstream ignored;if(NoqeriDatabase{}.executeSql(bad,database,ignored,badD)){std::filesystem::remove_all(root);return 3;}
    const auto check=root/"check.nqd";{std::ofstream out(check);out<<"select users where id = 1\n";}
    Diagnostics checkD;std::ostringstream checkOut;if(!NoqeriDatabase{}.execute(check,database,checkOut,checkD)){checkD.print(check.string());std::filesystem::remove_all(root);return 4;}
    if(checkOut.str().find("Ada Lovelace")==std::string::npos || checkOut.str().find("Broken")!=std::string::npos){std::cerr<<checkOut.str();std::filesystem::remove_all(root);return 5;}
    std::filesystem::remove_all(root);
    std::cout<<"SQL adapter checks passed\n";
    return 0;
}
