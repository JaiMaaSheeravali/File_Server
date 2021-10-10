#include <iostream>
#include <sys/socket.h>


#include "../include/Request.hpp"
#include "../include/color.hpp"
#include "../include/authentication.hpp"

using namespace std;

void debug_error(sql::SQLException &e){
    cout << "# ERR: SQLException in " << __FILE__;
    cout << "# ERR: " << e.what();
    cout << " (MySQL error code: " << e.getErrorCode();
    cout << ", SQLState: " << e.getSQLState() << " )" << endl;
}

sql::Connection * connect_db(){
    const string url = "localhost";
    const string user = "root";
    const string pass1 = "naman";
    const string database = "file_server";
    sql::Driver *driver;
    sql::Connection *myConn;
    
    try{
        driver = get_driver_instance();

        myConn = driver->connect(url, user, pass1);
        myConn->setSchema(database);
    }
    catch(sql::SQLException &e){
        debug_error(e);
    }

    return myConn;
}

bool Request::register_user(string username, string password){
    try{
        sql::Connection *myConn = connect_db();
        sql::Statement *myStatement;
        sql::ResultSet *myResult;
        // recieve the username password
        string query = "INSERT INTO ACCOUNT(username, password) VALUES('"+username+"','"+password+"')";
        myStatement = myConn->createStatement();
        myStatement->execute(query);
        cout << "Created the user " + username << endl;
        return true;
    }
    catch(sql::SQLException &e){
        debug_error(e);
    }
    return false;

}

bool Request::login_user(string username, string password){
    try{
        sql::Connection *myConn = connect_db();
        sql::Statement *myStatement;
        sql::ResultSet *myResult;
        // recieve the username password
        string query = "SELECT * FROM ACCOUNT WHERE username='" + username + "' AND password='"+ password + "'";
        myStatement = myConn->createStatement();
        myResult = myStatement->executeQuery(query);
        
        if(myResult->next()){
            cout << "User logged in" << endl;
            return true;
            // send affirmation to change the directory to user directory
        }
        else{
            cout << "User does not exist" << endl;
        }
    }
    catch(sql::SQLException &e){
        debug_error(e);
    }
    return false;

}
