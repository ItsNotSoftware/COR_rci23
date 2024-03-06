import socket


def send_message(host, port, message):
    try:
        # Create a TCP socket
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        print("1")
        # Connect to the server
        sock.connect((host, port))
        print("2")

        # Send the message
        sock.sendall(message.encode())
        print("3")

        # Receive response from the server
        response = sock.recv(1024).decode()
        print("Response:", response)

    except Exception as e:
        print("Error:", e)

    finally:
        # Close the socket
        sock.close()


# Specify the host and port to connect to
host = "127.0.0.1"  # Change this to the appropriate host
port = 5001  # Change this to the appropriate port

# Specify the message to send
message = "Hello, server! This is a test message."

# Call the function to send the message
send_message(host, port, message)
