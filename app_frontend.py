import flet as ft
import paho.mqtt.client as mqtt
import random

MQTT_BROKER = "broker.hivemq.com"

TOPIC_ESTADO = "clase/decoder/grupo_b7a/estado"
TOPIC_CONTROL = "clase/decoder/grupo_b7a/control"


def main(page: ft.Page):
    page.title = "Panel de Control ESP32 - Grupo B7A"
    page.theme_mode = ft.ThemeMode.DARK
    page.padding = 20
    page.window.width = 450
    page.window.height = 680

    indicador = ft.Text("● Desconectado", color=ft.Colors.ORANGE_400, size=14)
    display = ft.Text("0", size=70, weight=ft.FontWeight.BOLD, color=ft.Colors.CYAN_200)
    visor_bits = ft.Text("Bits: [ 0 0 0 0 ]", size=18, color=ft.Colors.GREY_400)
    label_origen = ft.Text("Origen: Esperando datos...", size=13, italic=True, color=ft.Colors.BLUE_200)

    def on_connect(client, userdata, flags, rc, properties=None):
        def actualizar():
            if rc == 0:
                indicador.value = "● Conectado al Broker"
                indicador.color = ft.Colors.GREEN_400
                client.subscribe(TOPIC_ESTADO)
            else:
                indicador.value = f"● Error de conexión (rc={rc})"
                indicador.color = ft.Colors.RED_400
            page.update()

        page.run_thread(actualizar)

    def on_message(_, __, msg):
        def actualizar():
            try:
                datos = msg.payload.decode().strip().split(",")
                if len(datos) != 2:
                    return

                binario, decimal_str = datos

                if decimal_str.isdigit() and len(binario) == 4:
                    display.value = decimal_str
                    visor_bits.value = f"Bits: [ {' '.join(binario)} ]"
                    label_origen.value = "Origen: DIP Switch (Hardware ESP32)"
                    page.update()

            except Exception as ex:
                print(f"[on_message] Error: {ex}")

        page.run_thread(actualizar)

    client_id = f"flet_client_{random.randint(1000, 9999)}"
    mqtt_client = mqtt.Client(client_id=client_id, protocol=mqtt.MQTTv311)
    mqtt_client.on_connect = on_connect
    mqtt_client.on_message = on_message

    mqtt_client.connect_async(MQTT_BROKER, port=1883, keepalive=60)
    mqtt_client.loop_start()

    def enviar_numero(e):
        num_str = str(e.control.data)
        mqtt_client.publish(TOPIC_CONTROL, num_str)
        display.value = num_str
        label_origen.value = f"Origen: Teclado Web (Comando {num_str})"
        page.update()


    def on_window_event(e):
        if e.data == "close":
            mqtt_client.loop_stop()
            mqtt_client.disconnect()
            page.window.close()

    page.window.prevent_close = True
    page.on_window_event = on_window_event

    tarjeta_display = ft.Container(
        content=ft.Column(
            [
                visor_bits,
                display,
                label_origen
            ],
            horizontal_alignment=ft.CrossAxisAlignment.CENTER,
            spacing=5
        ),
        bgcolor=ft.Colors.BLUE_GREY_900,
        padding=20,
        border_radius=15
    )

    grid_botones = ft.GridView(
        expand=False,
        runs_count=5,
        max_extent=75,
        child_aspect_ratio=1.0,
        spacing=10,
        run_spacing=10,
    )

    for i in range(10):
        grid_botones.controls.append(
            ft.OutlinedButton(
                content=ft.Text(str(i), color=ft.Colors.CYAN_100),
                data=i,
                on_click=enviar_numero,
                style=ft.ButtonStyle(
                    shape=ft.RoundedRectangleBorder(radius=10),
                )
            )
        )

    page.add(
        ft.Column(
            [
                ft.Row([indicador], alignment=ft.MainAxisAlignment.CENTER),
                ft.Divider(height=10, color=ft.Colors.TRANSPARENT),
                tarjeta_display,
                ft.Divider(height=20, color=ft.Colors.GREY_800),
                ft.Text("Inyectar Valor al Hardware:", size=14, weight=ft.FontWeight.W_500),
                grid_botones
            ],
            spacing=15
        )
    )

ft.app(target=main)