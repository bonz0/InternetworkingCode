CSE678 Internetworking Code for Assignments and Project
=======================================================

Author: Farhang Zia  
Graduate Student  
*Department of Computer Science and Engineering,*  
*The Ohio State University, Columbus.*

Contact email: [farhang.bonzo@gmail.com](mailto:farhang.bonzo@gmail.com).

--------------------------------

About:
------
Coding assignments and project for CSE678 Internetworking class.
Instructions for the project:
	In the main Project folder, do 'make clean' and then 'make'.
	In the timer_code folder, do 'make clean' and then 'make'.
	execute the linux_troll using the following settings:
		./linux_troll -x25 -g25 -se10 -r -m10 9999
	execute the following in order:
		./ftps <server's port number>
		./tcpd_server
		./tcpd_client
		./timerprocess
		./ftpc <server's ip address> <server's port number> <file name>
