import socket      #import socket module to allow code to connect to cart
import subprocess  #import subprocess module to allow command terminal calls for digicam

TCP_IP = '192.168.100.94' #IP address of ethernet connection to the cart                         
TCP_PORT = 5006        #Port 5002 - expects ASCII commands, use port 5006 if you need to communicate with controller more interactively

DIGICAM_PATH = "C:\Program Files (x86)\digiCamControl\CameraControlCmd.exe"  #Path to the CameraControlCmd.exe file needed to trigger photos

class Cart:
    def __init__(self, ip: str, port: int): #defines function cart and sets first input to be the IP address, second input to be the port used
        self.ip = ip
        self.port = port
        self.s = None

    def send_command(self, cmd: bytearray):                  #function to send command to cart
        if self.s:
#           self.s.send(bytes(cmd, 'utf-8') + b'\r\n')      #sends the command as a byte to the controller for Port 5006
            self.s.send(cmd)                                 #sends the command as a byte to the controller for Port 5002

    def get_command(self, request: bytearray):
        if self.s:
            self.s.send(request)
            respons = self.s.recv(1024)
        return respons

    def take_digicam_photo(self, path):                     #function to take photos - not needed to move cart
        out = subprocess.check_output(                      #sends code to windows command prompt
            [DIGICAM_PATH, "/folder", path, "/captureall"], #command to take picture
            )
        decoded = out.decode('utf-8')
        responses_split = decoded.split('\r\n')
        
        for response in responses_split:
            print(response) #prints the output of the command to the terminal so user can see if photos were taken
        
    def connect(self):                                              #function to connect to cart
        self.s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)  #sets the address to be IPV4 and the connection to be TCP protocal
        self.s.connect((self.ip, self.port))                        #calling connect uses the inputs provided when calling Cart

    def disconnect(self):
        self.s.close()