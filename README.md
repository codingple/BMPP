# BMPP
#### Bonjour Monde! Peer to Peer (BMPP) : A simple P2P program on Linux  

### Description
This is a Peer to Peer file transfer program with a broker server on Linux. At the first, bmpp_cl.c, as a client of this program,
connects to the broker, bmpp_sv.c. The server keeps the connection from the clients, receives a file request with the file name,
asks whether the clients have the file, and give the address of the file owner to who needs it. The client who gets the address 
connects to the peer and receives the file.  

### Usage
* **Server**  

So that this application has to use _port 9190_, open the port with:  
>> sudo ufw allow 9190/tcp  
  
Compile "bmpp_sv.c" using the following:  
>> gcc -o sv bmpp_sv.c  
  
Excute the server:  
>> ./sv  
  
  
* **Client**  
  
For the file transfer, as well, open the port with:  
>> sudo ufw allow 919/tcp  
  
Compile "bmpp_cl.c" using the following:  
>> gcc -o cl bmpp_cl.c  
  
Excute the client:  
>> ./cl  
  
Input the IP address of the server, like:  
>> 10.223.32.22  
  
Now, "BMPP" directory is bone next to the client application(e.g. in the same directory)  
You and other peer can share any file in the directory with file name.  
Type the file name to acquire, and you can receive the file if any peer share the file, like:  
>> test.txt  

