import paramiko
import scp

ver = "10.1.1.6"

try:
    client = paramiko.SSHClient()
    client.load_system_host_keys()
    client.connect("titan", username = "shyam", password = "helper")
    ssh_in, ssh_out, ssh_err = client.exec_command("cd appa/test_folder; cat version.txt")
    ssh_out.channel.recv_exit_status()

    output = ""
    for line in ssh_out:
        output += line

    if(output[9:] != ver + "\n"):
        print("Updating...")

        scp_client = scp.SCPClient(client.get_transport())
        scp_client.put("/home/shyam/appa/train_folder/file.txt", remote_path = "/home/shyam/appa/test_folder/file_new.txt", recursive = True)
        scp_client.close()
        ssh_in, ssh_out, ssh_err = client.exec_command("cd appa/test_folder; cat file_new.txt")
        lines1 = ssh_out.readlines()

        with open("file.txt") as f:
            lines2 = f.readlines()

        if(lines1 == lines2):
            cmds = ["echo version: " + ver + " > /home/shyam/appa/test_folder/version.txt", "rm /home/shyam/appa/test_folder/file.txt", "mv /home/shyam/appa/test_folder/file_new.txt /home/shyam/appa/test_folder/file.txt"]
            for cmd in cmds:
                client.exec_command(cmd)
        client.close()
    else:
        print("Upto date.")

except Exception as e:
    print("Error: " + str(e))
