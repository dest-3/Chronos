import socket
import time

SERVER_PORT = 5000 
HEARTBEAT_WINDOW = 180  # seconds
MAX_EVENT_LOGS = 0

api_map = {
    1: "xTaskCreate", 2: "xTaskCreateStatic", 3: "xTaskCreateRestricted", 4: "xTaskCreateRestrictedStatic", 
    5: "vTaskDelete", 6: "xTaskAbortDelay", 7: "uxTaskPriorityGet", 8: "eTaskGetState", 9: "vTaskGetInfo", 
    10: "vTaskPrioritySet", 11: "vTaskSuspend", 12: "xTaskGetHandle", 13: "uxTaskGetStackHighWaterMark", 
    14: "vTaskSetApplicationTaskTag", 15: "xTaskGetApplicationTaskTag", 16: "vTaskSetThreadLocalStoragePointer", 
    17: "pvTaskGetThreadLocalStoragePointer", 18: "xTaskCallApplicationTaskHook", 19: "xTaskGenericNotify", 
    20: "xTaskNotifyStateClear", 25: "vTaskAllocateMPURegions", 
    21: "vTaskDelayUntil - RET", 22: "vTaskDelay - RET", 23: "vTaskSuspend - RET", 24: "taskYIELD_FROM_TASK - RET"
}

def parse_event(message): 
    if "USER" in message: 
        try:
            api_index = int(message.split("Val: ")[1])
            api_name = api_map[int(api_index)]
            return f"{message} - {api_name}"
        except (IndexError, ValueError):
            return f"{message} - Invalid API Index"
        
    return message

class UDPServer:
    def __init__(self, host, port, timeout_interval, max_logs):
        self.host = host
        self.port = port
        self.timeout_interval = timeout_interval
        self.max_logs = max_logs
        self.num_logs = 0
        self.log_file = "log.txt"
        self.timeout_alert = f"No message received in the last {self.timeout_interval} seconds."

    def write_log(self, log, entry):
        if self.num_logs < self.max_logs and "Heartbeat" not in entry:
            log.write(entry)
            log.flush()
            self.num_logs += 1

    def run(self):
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as server_socket:
            server_socket.bind((self.host, self.port))
            print(f"UDP server is listening on {self.host}:{self.port}")
            server_socket.settimeout(self.timeout_interval)

            with open(self.log_file, "a") as log:
                print(f"Logging messages to {self.log_file}")

                while True:
                    try:
                        data, addr = server_socket.recvfrom(1024)
                        message = data.decode("utf-8")
                        timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
                        parsed_message = parse_event(message)
                        log_entry = f"{timestamp} - Received from {addr}: {parsed_message}\n"

                        print(f"Logged message: {log_entry.strip()}")
                        self.write_log(log, log_entry)

                    except socket.timeout:
                        timeout_entry = f"{time.strftime('%Y-%m-%d %H:%M:%S')} - {self.timeout_alert}\n"
                        print(timeout_entry)
                        self.write_log(log, timeout_entry)

if __name__ == "__main__":
    server = UDPServer("192.168.4.100", SERVER_PORT, HEARTBEAT_WINDOW, MAX_EVENT_LOGS)
    server.run()
