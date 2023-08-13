from pipython import GCSDevice, pitools
import serial

def getAxis(in1, in2):
    if(in2 == '0'):
        isAngle = False
    elif(in2 == '1'):
        isAngle = True
    else:
        print(f"Bad angle char {in2}")
    match(in1):
        case '0':
            return 'U' if isAngle else 'X'
        case '1':
            return 'V' if isAngle else 'Y'
        case '2':
            return 'W' if isAngle else 'Z'
        case _:
            print(f"Unknown axis char {in1}")
            return 0

def printCurrentPos(pidevice,digits = 3):
    currentPos = pidevice.qPOS()
    print(''.join([f' {k},{v:.3f}' for k,v in currentPos.items()]))

def decodeAndExecuteString(strIn):
    strParts = strIn.split(' ')
    #if(len(strParts) != 0):
    if(strParts[0] == 'STEP'):
        axisVal = getAxis(strParts[1],strParts[2])
        if(axisVal != 0):
            stepDist = float(strParts[3])
                #print("Step forward" + axisVal)
            #print(axisVal, stepDist)
            pidevice.MVR(axisVal, stepDist)
            #pitools.waitontarget(pidevice)
            #printCurrentPos(pidevice)
            # if(stepDist < 0):
            #     print("Step backward" + axisVal)
            #     pidevice.MVR(axisVal, -1.0)

if __name__ == '__main__':
    with GCSDevice('C-887') as pidevice:
        # Choose the interface according to your cabling.
        pidevice.ConnectTCPIP(ipaddress='169.254.11.63')
        print('connected: {}'.format(pidevice.qIDN().strip()))
        if pidevice.HasqVER():
            print('version info:\n{}'.format(pidevice.qVER().strip()))
        with serial.Serial('COM15', 38400, timeout = 1) as ser:
            while True:
                cc = str(ser.readline().decode("utf-8").rstrip())
                if(cc != ''):
                    decodeAndExecuteString(cc)
