from cgitb import text
from genericpath import samefile
import os
from tkinter import *
from tkinter import ttk
from typing import Literal
from unittest.mock import DEFAULT
from PIL import ImageTk, Image
# Bibliotecas matematicas/Plot
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg, NavigationToolbar2Tk
import matplotlib.animation as animation

#Serial + Time
import datetime
import time
import serial
import serial.tools.list_ports

#for struct unpack
import struct

import webbrowser

# Criando a interface
app = Tk()
app.title("Reflow oven GUI")
app.geometry("1280x700")
#app.iconbitmap('figuras/INEPlogo.ico')

### Menus
def donothing():
    print('donothing')

def save_fig():
    figura.savefig('Grafico.png')

def clear_txt():
    ax.clear()
    ax.xaxis.set_major_locator(plt.MaxNLocator(8))
    ax.yaxis.set_major_locator(plt.MaxNLocator(10))
    ax.set_xlabel('Time')
    ax.set_ylabel('Temperature (ºC)')
    ax.set_title('Climatic Data')
    ax.legend(["Referência","Temperatura"])
    ax.tick_params(axis = 'x',rotation = 0)

    line1, = ax.plot(0,0, label='Perfil Temperatura', color='r', linewidth='2.0')
    line2, = ax.plot(0,0, label='Temperatura Atual', color='b', linewidth='1.0')

    canvas.draw()

    open('sampleData.txt', 'w').close()


menubar = Menu(app)
filemenu = Menu(menubar, tearoff=0)
filemenu.add_command(label="Clear Data", command=clear_txt)
filemenu.add_command(label="Open", command=donothing)
filemenu.add_command(label="Save", command=save_fig)
filemenu.add_command(label="Close", command=donothing)

filemenu.add_separator()

filemenu.add_command(label="Exit", command=app.quit)
menubar.add_cascade(label="File", menu=filemenu)

# Cria notebook
nb = ttk.Notebook(app)
nb.place(x=0,y=0,width=800,height=600)
nb.pack(expand=True, fill=BOTH)
# Cria as abas e anexa ao notebook
############################ Comunicação ########################
tab1 = Frame(nb)
nb.add(tab1,text="Comunicação")

is_on = False

####Inicia Serial
class Communication:

    def __init__(self):     

        import serial
        import serial.tools.list_ports

        self.serial   = serial.Serial()

        self.serial.port     = 'COM3'
        self.serial.baudrate = 115200
        self.serial.bytesize = serial.EIGHTBITS
        self.serial.parity   = serial.PARITY_NONE
        self.serial.stopbits = serial.STOPBITS_ONE
        self.serial.timeout  = 0.1
    
    def is_open(self) -> bool:
        return self.serial.is_open

    def open(self) -> None:
        self.serial.open()   

    def close(self) -> None:
        self.serial.close()

    def write(self, string: str) -> None:
        self.serial.write(string)   

    def read(self, lenght: Literal) -> str:
        return self.serial.read(lenght)

    def getPorts(self) -> str:        
        ports = serial.tools.list_ports.comports()
        temp = []
        for port, desc, hwid in sorted(ports):
            temp += ["{}: {} [{}]\n".format(port, desc, hwid)]
        return temp

    def setCOM(self, COMport: str) -> bool:
        self.serial.port = COMport
    
communication = Communication()

ProfileName, ProfileUnit, ProfileDataType = (0, 1, 2)
ProfileDescription =    [['ReflowData'          ,'°C/s', 'f'],
                        ['PreheatSoakInitTemp'  ,'°C'  , 'f'],
                        ['PreheatSoakEndTemp'   ,'°C'  , 'f'],
                        ['PreheatSoakTime'      ,'s'   , 'f'],
                        ['PreheatSoakRate'      ,'°C/s', 'f'],
                        ['RampToReflow'         ,'°C/s', 'f'],
                        ['ReflowTemp'           ,'°C'  , 'f'],
                        ['ReflowTime'           ,'s'   , 'f'],
                        ['CoolingRate'          ,'°C/s', 'f']]

DataSizeInByte = 4
Profile = [0] * len(ProfileDescription)

ReflowName, ReflowUnit, ReflowDataType = (0, 1, 2)
ReflowDescription = [['TimeFromStart'       , 's'    , 'f'],
                     ['reflow_Temp_raw'     , '°C'   , 'f'],
                     ['reflow_Temp_filtered', '°C'   , 'f'],
                     ['reflow_ref'          , '°C'   , 'f'],
                     ['reflow_duty'         , 'None' , 'f'],
                     ['reflow_status'       , 's'    , 'I']]
Reflow = [0] * len(ReflowDescription)

def PortsPrint():
    TextScroll.insert(END,communication.getPorts())    

def abrirPorta():
    #ser.port = PortSerial.get()
    communication.setCOM(PortSerial.get())
    TextScroll.insert(END,'Port: ' + PortSerial.get() + ' selected\n')
    
    if communication.is_open() :
        TextScroll.insert(END,'Port already open\n')
    else:
        try:
            communication.open()
            time.sleep(3)
            TextScroll.insert(END,'Port open\n')
        except:
            TextScroll.insert(END,'Fail to connect port: ' + PortSerial.get() + '\n')
        
def fecharPorta():
    if communication.is_open():
        communication.close()
        TextScroll.insert(END,'Port closed\n')
    else:
        TextScroll.insert(END,'Port already closed\n')

def get_reflow_data():
    if communication.is_open():
        communication.write(b'D')
        data = communication.read(DataSizeInByte * len(Reflow))
        for i in range(0,len(Reflow)):
            Reflow[i] = struct.unpack(ReflowDescription[i][ReflowDataType], data[0 + DataSizeInByte * i: DataSizeInByte + DataSizeInByte * i])

def clean_terminal():
    TextScroll.delete('0.0', END)

def get_profile_func(): 
    if communication.is_open():
        communication.write(b'R')
        data = communication.read(DataSizeInByte * len(ProfileDescription))
        for i in range(0,len(ProfileDescription)):
            Profile[i] = struct.unpack('f', data[0 + DataSizeInByte * i: DataSizeInByte + DataSizeInByte * i])
            TextScroll.insert(END, ProfileDescription[i][ProfileName] + ' = ' + str(Profile[i][0]) + ProfileDescription[i][ProfileUnit] + '\n')
    else:
        TextScroll.insert(END,'Port closed \n')

def test_comunication():
    
    if is_on:
        TextScroll.insert(END,'Acquisition in progress, STOP first \n')
        return
    if communication.is_open():
        #Request data
        communication.write(b'o')
        data = communication.read(2)

        if data == b"OK":            
            TextScroll.insert(END,'Communication is active \n')
        else:
            TextScroll.insert(END,'Fail to communicate \n')    
    else:
        TextScroll.insert(END,'Port closed \n')

def start_stop_sample_func():

    global is_on
    if communication.is_open():
        if is_on:
            start_stop_sample['text'] = 'Start Acquisition'
            start_stop_sample['bg'] = 'lightgreen'
            is_on = False
            TextScroll.insert(END,'Acquisition stoped\n')
        else:
            start_stop_sample['text'] = 'Stop Acquisition'
            start_stop_sample['bg'] = 'red'
            is_on = True
            TextScroll.insert(END,'Acquisition has started\n')
    else:
        TextScroll.insert(END,'Port is closed\n')

def start_reflow_func():
    if communication.is_open():
        TextScroll.insert(END,'Reflow started\n')
        communication.write(b'1')
    else:
        TextScroll.insert(END,'Port is closed\n')

def stop_reflow_func():
    if communication.is_open():
        TextScroll.insert(END,'Reflow stoped\n')
        communication.write(b's')
    else:
        TextScroll.insert(END,'Port is closed\n') 

Label(tab1,text="List COM ports:",anchor=W).place(x=0,y=0,width=200,height=30)    
GetPorts = Button(tab1,text="List",command = PortsPrint)
GetPorts.place(x=100,y=0,width=100,height=30)
ScrollText = Scrollbar(tab1)
TextScroll = Text(tab1,height=10,width=50)
ScrollText.place(x=610,y=0,width=25,height=210)    
TextScroll.place(x=210,y=0,width=400,height=210)
ScrollText.config(command=TextScroll.yview)
TextScroll.config(yscrollcommand = ScrollText.set)
Label(tab1,text="Enter chamber COM port:",anchor=W).place(x=0,y=30,width=200,height=30)  
PortSerial = Entry(tab1)
PortSerial.insert(0,'COM3')
PortSerial.place(x=0,y=60,width=200,height=30)

#obsoleto
#setPort = Button(tab1,text="Select Port",command = PortAtualization)
#setPort.place(x=0,y=90,width=200,height=30)
openPort = Button(tab1,text="Open Port",command = abrirPorta)
openPort.place(x=0,y=90,width=100,height=30)
closePort = Button(tab1,text="Close Port",command = fecharPorta)
closePort.place(x=100,y=90,width=100,height=30)
clean = Button(tab1,text="Test Comunication",command = test_comunication)
clean.place(x=0,y=120,width=200,height=30)
test_com = Button(tab1,text="Clean terminal",command = clean_terminal)
test_com.place(x=0,y=150,width=200,height=30)
get_profile = Button(tab1,text="Get profile",command = get_profile_func)
get_profile.place(x=0,y=180,width=200,height=30)
start_stop_sample = Button(tab1,text="Start Acquisition",command = start_stop_sample_func , bg='green')
start_stop_sample.place(x=0,y=210,width=200,height=30)

start_reflow = Button(tab1,text="Start Reflow",command = start_reflow_func , bg='green')
start_reflow.place(x=0,y=240,width=100,height=30)

stop_reflow = Button(tab1,text="Stop Reflow",command = stop_reflow_func , bg='red')
stop_reflow.place(x=100,y=240,width=100,height=30)
######################## Gráficos #########################
tab2 = Frame(nb)
nb.add(tab2,text="Gráficos")



# Figura        
figura = plt.Figure(figsize=(5,5), dpi=100)
ax  = figura.add_subplot(1,1,1)

#Data import

#Se não existe, cria
with open('sampleData.txt','a'):
    pass

with open('sampleDataInfo.txt','w') as pullData:
    #Place header description
    pullData.write('[ReflowName , ReflowUnit , ReflowDataType]\n')
    #Populate info
    for i in range(0,len(Reflow)):
        pullData.write('%s \n' % ReflowDescription[i])  
   


line1, = ax.plot(0,0, label='Perfil Temperatura', color='r', linewidth='2.0')
line2, = ax.plot(0,0, label='Temperatura Atual', color='b', linewidth='1.0')

ax.xaxis.set_major_locator(plt.MaxNLocator(8))
ax.yaxis.set_major_locator(plt.MaxNLocator(10))
ax.set_xlabel('Time')
ax.set_ylabel('Temperature (ºC)')
ax.set_title('Climatic Data')
ax.legend(["Referência","Temperatura"])
ax.tick_params(axis = 'x',rotation = 0)

canvas = FigureCanvasTkAgg(figura,tab2)
canvas.draw()
canvas.get_tk_widget().pack(side=TOP, fill=BOTH, expand=True)
toolbar = NavigationToolbar2Tk(canvas,tab2)
toolbar.update()
canvas.get_tk_widget().pack(side=TOP, fill=BOTH, expand=1)

Label(tab2,text="Temperatura :",anchor=W,background="white").place(x=0,y=0,width=100,height=20) 
temp_value = Label(tab2, text='0',background="white")
temp_value.place(x=100,y=0,width=40,height=20)
Label(tab2,text="Perfil :",anchor=W,background="white").place(x=0,y=20,width=100,height=20) 
perfil_value = Label(tab2, text='0',background="white")
perfil_value.place(x=100,y=20,width=40,height=20)

def run_figure(i):

    global is_on

    if is_on == True:
        
        get_reflow_data()

        with open('sampleData.txt','a') as pullData:
            Tempo = datetime.datetime.now().strftime("%H:%M:%S") #"%Y-%m-%d %H:%M:%S"
            pullData.write('%s,' % Tempo)           # Time From app
            pullData.write('%f,' % Reflow[0][0])    # TimeFromStart
            pullData.write('%f,' % Reflow[1][0])    # reflow
            pullData.write('%f,' % Reflow[2][0])    # reflow_Temp_filtered
            pullData.write('%f,' % Reflow[3][0])    # reflow_ref
            pullData.write('%f,' % Reflow[4][0])    # reflow_duty
            pullData.write('%s'  % Reflow[5][0])    # reflow_status
            pullData.write('\n')

        Data = pd.read_csv('sampleData.txt',header=None)
        X = Data.to_numpy()
        
        TempoList = X[:,0]
        TempAList = X[:,3]
        TempDList = X[:,4]   

        #update box
        temp_value.config  (text = TempAList[-1])
        perfil_value.config(text = TempDList[-1])
        
        ax.plot(TempoList, TempDList, label='Perfil Temperatura', color='r', linewidth='1.0')
        ax.plot(TempoList, TempAList, label='Temperatura Atual', color='b')

        line1.set_xdata(TempoList)    
        line1.set_ydata(TempDList)
        line2.set_xdata(TempoList)
        line2.set_ydata(TempAList)

        ax.draw_artist(ax.patch)
        ax.draw_artist(line1)
        
        figura.canvas.draw()
        figura.canvas.flush_events()


 

tab3 = Frame(nb)
nb.add(tab3,text="Relatório")
# Create an object of tkinter ImageTk
img = ImageTk.PhotoImage(Image.open("figuras/profile.png"))

# Create a Label Widget to display the text or Image
reflow_img = Label(tab3, image = img)
reflow_img.place(x=0,y=0)

reflow_img.bind("<Button-1>", lambda e: webbrowser.open_new("https://www.nxp.com/docs/en/supporting-information/Reflow_Soldering_Profile.pdf"))


def calculate_func():
    if communication.is_open():
        get_profile_func()

        RampUpRate.delete('0', END)
        RampUpRate.insert('0', str(Profile[0][0]))
        PreheatSoakInitTemp.delete('0', END)
        PreheatSoakInitTemp.insert('0',str(Profile[1][0]))
        PreheatSoakEndTemp.delete('0', END)
        PreheatSoakEndTemp.insert('0',str(Profile[2][0]))
        PreheatSoakTime.delete('0', END)
        PreheatSoakTime.insert('0',str(Profile[3][0]))

        TempMaintainedAbove.delete('0', END)
        TempMaintainedAbove.insert('0','183.0')
        TimeTl =  (Profile[6][0] - 183.0 ) / Profile[5][0] + Profile[7][0] + (Profile[6][0] - 183.0 ) / Profile[8][0]
        TimeMaintainedAbove.delete('0', END)
        TimeMaintainedAbove.insert('0',str(float(f'{TimeTl:.1f}')))
        ReflowTemp.delete('0', END)
        ReflowTemp.insert('0',str(Profile[6][0]))

        
        TempPeak.delete('0', END)
        TempPeak.insert('0',str(Profile[7][0]))
        CoolingRate.delete('0', END)
        CoolingRate.insert('0',str(Profile[8][0]))
        TimeToPeakCalc = ((Profile[1][0] - 25.0 ) / Profile[0][0] + Profile[3][0] + (Profile[6][0] - Profile[2][0]) / Profile[5][0] )
        m, s = divmod(TimeToPeakCalc, 60)
        TimeToPeak.delete('0', END)
        TimeToPeak.insert('0',str(int(m)) +'\'' + str(int(s)) + '\"')

calculate = Button(tab3, text="Calculate", command = calculate_func)
calculate.place(x=590,y=0,width=100,height=30)

RampUpRate = Entry(tab3)
RampUpRate.place(x=590,y=75,width=100,height=20)

PreheatSoakInitTemp = Entry(tab3)
PreheatSoakInitTemp.place(x=590,y=95,width=100,height=20)
PreheatSoakEndTemp = Entry(tab3)
PreheatSoakEndTemp.place(x=590,y=115,width=100,height=20)
PreheatSoakTime = Entry(tab3)
PreheatSoakTime.place(x=590,y=135,width=100,height=20)

TempMaintainedAbove = Entry(tab3)
TempMaintainedAbove.place(x=590,y=175,width=100,height=20)
TimeMaintainedAbove = Entry(tab3)
TimeMaintainedAbove.place(x=590,y=195,width=100,height=20)

ReflowTemp = Entry(tab3)
ReflowTemp.place(x=590,y=215,width=100,height=20)

TempPeak = Entry(tab3)
TempPeak.place(x=590,y=255,width=100,height=20)
CoolingRate = Entry(tab3)
CoolingRate.place(x=590,y=275,width=100,height=20)
TimeToPeak = Entry(tab3)
TimeToPeak.place(x=590,y=295,width=100,height=20)



ani = animation.FuncAnimation(figura, run_figure , interval=1000 )
app.config(menu=menubar)
app.mainloop()

