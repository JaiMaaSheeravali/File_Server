CSN 510: File Server

## Steps to run locally:-

1. Clone the repo and move inside File_Server directory
2. `sudo apt update`
3. `sudo apt install build-essential`
4. `sudo apt-get install libmysqlcppconn-dev`
5. `cat instruct.txt | mysql -u root -p`
6. Type the mysql password. Database would be created along with the table
7. Now go to `File_Server/src/authentication.cpp` and replace the empty string for variable `pass1` with the mysql password. After that go to parent directory of the project.
8. `cmake .`
9. `make`
10. `./output/server`
