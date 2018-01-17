# TemperatureRelay

Collect and report the temperatures of a remote location.

A mishmash of C, Python, PHP, SQL, HTML, CSS, and Javascript which somehow works together.

Results are viewable via a dynamically generated webpage.

## General overview

Remote hardware consists of an ESP8266 with a DHT temperature sensor, and an internet connection.

The server side consists of a Python TCP gateway and a LAMP server.

The ESP8266 reads the DHT at regular intervals, sends its data to the Python gateway, which then updates the SQL database. The LAMP server is used to interface with the SQL database, and to serve a webpage displaying the collected data.

## Features

* Light network load
* Intelligent handling of network loss
* Basic password security for all connections and forms with database write access
* CSV download of data, with basic time filter

## Network load

The ESP8266 and Python gateway are designed to use as little data as possible. The TCP session is kept alive for as long as possible, and the application's use of the TCP connection is very simple, almost naive.

With a temperature check-in every 10 minutes, I estimate that the ESP will use around 850KB per month. This means that internet connections with very low datacaps can be used. In my setup I am using a cellular hotspot with [Hologram](https://hologram.io/) cell service. Thus, my monthly connection cost is under $1.

## Design choices

Throughout this project there are a variety of odd design choices. These primarily are due to the systems which I currently have access to. 

For such a simple system, a full LAMP stack seems a little overkill. However, the LAMP setup was already available and configured through services provided by my University.

Since the LAMP setup is run by my University, there is a very aggressive firewall between it and the internet. Port 80 HTTP/HTTPS is about the only thing it lets through. Thus, the Python gateway was made to run on a separate system, and to report the data through a HTTP form POST.
