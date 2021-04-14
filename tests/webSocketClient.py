import time
import websocket
def checkEcho(conn, value):
    print ('Sending ', value)
    conn.send(value)
    time.sleep(0.1)
    result = str(conn.recv())
    print ('Received', result)
    if result != value:
        print ('Error incorrect response')
        return False
    else:
        return True

conn = websocket.create_connection('ws://localhost:8080')
print (str(conn.recv()))
checkEcho(conn, 'Hello!')
checkEcho(conn, 'This is other text with no ASCII chars ññññáéÇ')
conn.send('close')
time.sleep(0.1)
print (str(conn.recv()))
conn.close()
conn = websocket.create_connection('ws://localhost:8080')
print (str(conn.recv()))
checkEcho(conn, 'Second try!')
checkEcho(conn, b'Text as bynary array')
conn.send('end')
time.sleep(0.1)
print (str(conn.recv()))
conn.close()
