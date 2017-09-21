import socket

host = '127.0.0.1'
port = 1115 

request = '*\r\nSHANG\r\nPING\r\n'

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((host, port))

s.sendall(request)
reply = s.recv(1024)
print '[client]reply:', reply
s.close()
