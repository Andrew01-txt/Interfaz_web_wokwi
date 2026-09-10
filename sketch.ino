import network
import time
from machine import Pin
from umqtt.simple import MQTTClient

WIFI_SSID = "Wokwi-GUEST"
WIFI_PASS = ""
MQTT_BROKER = "broker.hivemq.com"
CLIENT_ID = "esp32_display_grupo_b7a"

TOPICO_MONITOREO = "clase/decoder/grupo_b7a/estado"
TOPICO_CONTROL = "clase/decoder/grupo_b7a/control"

def conectar_wifi():
    wlan = network.WLAN(network.STA_IF)
    wlan.active(True)
    wlan.connect(WIFI_SSID, WIFI_PASS)

    inicio = time.ticks_ms()
    while not wlan.isconnected():
        if time.ticks_diff(time.ticks_ms(), inicio) > 10000:
            raise OSError("WiFi timeout - verifique la red en Wokwi")
        time.sleep_ms(300)
        print("  Esperando IP...")

    print("WiFi OK - IP:", wlan.ifconfig()[0])
    return wlan

segment_pins = {
    'A': Pin(15, Pin.OUT),
    'B': Pin(2, Pin.OUT),
    'C': Pin(4, Pin.OUT),
    'D': Pin(5, Pin.OUT),
    'E': Pin(18, Pin.OUT),
    'F': Pin(19, Pin.OUT),
    'G': Pin(21, Pin.OUT)
}

digit_map = {
    0: (1, 1, 1, 1, 1, 1, 0),
    1: (0, 1, 1, 0, 0, 0, 0),
    2: (1, 1, 0, 1, 1, 0, 1),
    3: (1, 1, 1, 1, 0, 0, 1),
    4: (0, 1, 1, 0, 0, 1, 1),
    5: (1, 0, 1, 1, 0, 1, 1),
    6: (1, 0, 1, 1, 1, 1, 1),
    7: (1, 1, 1, 0, 0, 0, 0),
    8: (1, 1, 1, 1, 1, 1, 1),
    9: (1, 1, 1, 1, 0, 1, 1)
}

def mostrar_numero(num):
    if num in digit_map:
        pattern = digit_map[num]
        segments = ['A', 'B', 'C', 'D', 'E', 'F', 'G']
        for i in range(7):
            segment_pins[segments[i]].value(pattern[i])

switch_pins = [
    Pin(32, Pin.IN, Pin.PULL_DOWN),
    Pin(33, Pin.IN, Pin.PULL_DOWN), 
    Pin(25, Pin.IN, Pin.PULL_DOWN), 
    Pin(26, Pin.IN, Pin.PULL_DOWN)  
]

def leer_dip_switch():
    valor = 0
    for i in range(4):
        valor += switch_pins[i].value() * (2 ** i)
    if valor > 9:
        valor = 9
    return valor

def al_recibir_del_frontend(topic, msg):
    try:
        numero_remoto = int(msg.decode())
        if 0 <= numero_remoto <= 9:
            mostrar_numero(numero_remoto)
            print("Comando remoto recibido:", numero_remoto)
        else:
            print("Valor fuera de rango:", numero_remoto)
    except ValueError:
        print("Mensaje no numérico recibido:", msg)

def conectar_mqtt():
    client = MQTTClient(CLIENT_ID, MQTT_BROKER)
    client.set_callback(al_recibir_del_frontend)
    client.connect()
    client.subscribe(TOPICO_CONTROL)
    print("MQTT listo - escuchando en:", TOPICO_CONTROL)
    return client

def on_connect(client, userdata, flags, rc, properties=None):
    def actualizar():
        if rc == 0:
            indicador.value = "● Conectado al Broker"
            indicador.color = ft.colors.GREEN_400
            client.subscribe(TOPIC_ESTADO)
        else:
            indicador.value = f"● Error de conexión (rc={rc})"
            indicador.color = ft.colors.RED_400
        page.update()

    page.run_thread(actualizar)


def on_message(client, userdata, msg):
    def actualizar():
        try:
            datos = msg.payload.decode().strip().split(",")
            if len(datos) != 2:
                return

            binario, decimal_str = datos

            if decimal_str.isdigit() and len(binario) == 4:
                display.value = decimal_str
                visor_bits.value = f"Bits: [ {' '.join(binario)} ]"
                page.update()

        except Exception as ex:
    
            print(f"[on_message] Error: {ex}")

    page.run_thread(actualizar) 


conectar_wifi()
cliente_mqtt = conectar_mqtt()

mostrar_numero(0)
ultimo_valor = -1

while True:
    
    cliente_mqtt.check_msg()

    valor_actual = leer_dip_switch()

    if valor_actual != ultimo_valor:
        bits_str = "".join([str(switch_pins[i].value()) for i in reversed(range(4))])
        payload = f"{bits_str},{valor_actual}"

        cliente_mqtt.publish(TOPICO_MONITOREO, payload.encode())
        print("Publicado:", payload)
        
        mostrar_numero(valor_actual)
        ultimo_valor = valor_actual

    time.sleep_ms(150)
