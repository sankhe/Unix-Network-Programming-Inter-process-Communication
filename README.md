# Unix-Network-Programming-Inter-process-Communication

●	Implemented  the file server and client processes to simulate the interprocess communication among them using IPC mechanisms like    
  PIPE,4 FIFO and Message Queues, Sockets(TCP and UDP).
●	The client process reads the input commands from the User i.e. command line and pass it to the file server through IPC mechanism.
●	Commands are generally in <Opcode filename> format. e.g read abc.txt 
●	File Server will return the result back to client process and client process will display the result to the User.
●	The interaction between User, Client and the File Server are synchronous, i.e. the user has to wait to receive the result of an
  operation before requesting another operation.
