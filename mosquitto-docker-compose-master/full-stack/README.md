## How to:
### MQTT:
First go into "C:\Users\krist\Git_folder\glowing-adventure\mosquitto-docker-compose-master\full-stack>"
Probably just "cd full-stack" and your good.
Now you are in the right place to run:
Start the docker container


```
docker-compose up -d mqtt
```
This starts a MQTT broker locally on your PC via docker:)
Then you lanch NodeRed on your PC or via docker:
PC via powershell: 
```
node-red
```
Docker:
On Windows
- Install [VcXsrv Windows X Server Download](https://sourceforge.net/projects/vcxsrv/) and double click [config.xlaunch](config.xlaunch) while in file explorer to run it. You can open file explorer by typing ```start .``` (include the period) in your terminal

```
docker-compose run install-node-red
docker-compose up -d nav
```
Node-RED will be hosted on http://localhost:1880 and the Node-RED UI will be hosted on http://localhost:1880/ui

Run the following command to shut everything down
```
docker-compose down
```




## Starting the entire stack
Run ```ipconfig``` in a terminal to find your ip address, and update the
existing ip address at the following path in [docker-compose.yml](docker-compose.yml)

You have to do this for each service that you wish to use. Below only demonstrates this for the turtlesim service.
```
services:
  turtlesim:
    environment:
      - DISPLAY=your_local_ip_address:0.0
```
On Windows
- Install [VcXsrv Windows X Server Download](https://sourceforge.net/projects/vcxsrv/) and double click [config.xlaunch](config.xlaunch) while in file explorer to run it. You can open file explorer by typing ```start .``` (include the period) in your terminal

```
docker-compose run install-node-red
docker-compose up -d nav
```
Node-RED will be hosted on http://localhost:1880 and the Node-RED UI will be hosted on http://localhost:1880/ui

Run the following command to shut everything down
```
docker-compose down
```

# Not my documentation (but very relevant):

## Hosting mqtt in docker

If you are on windows, open the ports in the firewall
   * NB! The following commands must be run in ```powershell```

Port 1883

```
New-NetFirewallRule -DisplayName "ALLOW TCP PORT 1883" -Direction inbound -Profile Any -Action Allow -LocalPort 1883 -Protocol TCP
```

Port 1901

```
New-NetFirewallRule -DisplayName "ALLOW TCP PORT 1901" -Direction inbound -Profile Any -Action Allow -LocalPort 1901 -Protocol TCP
```

Start the docker container

```
docker-compose up -d mqtt
```

Shut down the docker container so stop hosting mqtt

```
docker-compose stop mqtt
```


### Starting the master ros node docker container

```
docker-compose up -d master
```

You can see the container running with the following command

```
docker ps
```

### Running commands in an active docker container

```
docker exec -it <container_name> bash
source ros_entrypoint.sh
rosrun <package_name> <package_node>
```

### Shutting down all docker containers

```
docker-compose down
```

### Shutting down a specific docker container

```
docker-compose down <container_name>
```


To shut everything down, close the terminals and run the following code to stop the docker containers that are running in the background

```
docker-compose down
```

## Removing everything
```
docker system prune --volumes
```

# Installing on raspberry pi
NB! If you use sudo to clone, all future
commands will have to be run by sudo

First clone the repository
```
git clone --recursive https://github.com/nosknut/arduino-project.git
git submodule update
```

Now enter the cloned repo and checkout the ```develop``` branch.
```develop``` is the branch where raspberry pi deployments are made
```
cd arduino-project
git fetch
git checkout develop
```

Now navigate into the directory with the [docker-compose.yml](./docker-compose.yml) file
```
cd the-free-elected-peoples-democracy-of-libertyland/full-stack/
```

Run the following to install the node-red dependencies
```
sudo docker-compose run install-node-red
```

Now start the stack

Unless you have made changes to the code,
this is the conly command you need to
start the project after a reboot
```
sudo docker-compose up -d nav
```

Run this if you want to shut down the stack
```
sudo docker-compose down
```

Run this if you have made updates to the develop branch
```
git reset origin/develop --hard
```

When the reset is done, run the following to bring up the stack.

NB! Rerunning ```install-node-red``` is important if you have added
nodes to node-red since you last synced with the develop branch 
```
sudo docker-compose run install-node-red
sudo docker-compose up -d nav
```

Run the following to delete everything
```
sudo docker system prune --volumes
```
