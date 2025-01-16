# DV2603 System Defined Networking
## Project : Creation on a loadbalancer in C 

This project comes from a school project in Software Design Networking course.
The goal of this project is to code a loadbalancer for an horizontal scalability of webservers as well as the whole virtualized environment arounded.
The loadbalancer is coded in C with the sockets library.

### Features : 
 - load balacing between the webserver (Round Robin Algorithm)
 - dynamic scale -> we can scale up and down dynamically
 - multiple clients handling
 - webservers monitoring -> receive the cpu loading every 10s

#### Project architecture : 
![project_architecture](https://github.com/user-attachments/assets/06326dfe-5e94-4b7f-ae8a-af5251c81964)

#### Network architecture : 
![project_network](https://github.com/user-attachments/assets/50168e4f-97b1-44ee-81e0-478fc293e3aa)
