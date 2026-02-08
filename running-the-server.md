# Running the Server

The mod requires someone to run the server on a computer that is reachable by everyone who will connect. There are many ways to do this. This guide describes setting it up on AWS because that's what I've been using when hosting, but there are definitely other options. However if you don't want to use AWS, you're a bit more on your own (for now).

## AWS EC2 Instance

This guide will walk you through getting the server running on an AWS EC2 instance. Creating an account is free and you should get some credits to cover costs when joining which will last for 6 months to a year.

### Create Security Group

The security group defines network rules and acts as a firewall for your instance. You only need to create one security group; all your instances will use the same one.

1. In the EC2 service, click Security Groups in the sidebar on the left.
1. Click Create security group. Give it whatever name and description you like.
1. Add inbound rules:
    1. Add a Custom TCP rule. For Port range, choose the port the server will run on (default is `23432`). For Source, choose `Anywhere-IPv4`.
    1. Add a Custom UDP rule. Choose the same Port range and Source as the previous rule.
    1. Add an SSH rule. For Source, choose `My IP`.
1. Click Create security group. (The default Outbound rules are fine and don't need to be edited.)

### Create Instance

1. In the EC2 service, click Instances in the sidebar on the left.
1. Click Launch instances. Give it whatever name you want.
1. Leave the AMI and Instance type at their defaults. At the time of writing, the AMI is `Amazon Linux 2023 kernel-6.1 AMI` and the instance type is `t3.micro`.
1. Select your key pair:
    1. If this is your first time running the server, click Create new key pair. Just give it a name and click Create key pair. This will download a `.pem` file, which you should keep in a safe place on your computer.
    1. If you've already created a key pair, just select it from the drop down.
1. Click Select existing security group and choose the group created in the last section.
1. Click Launch instance. (Default values on everything else should be fine.) You can click the instance id to take you to the instance summary page.

### Run the Server

Now that the instance is running, we're ready to connect to it and run the server.

1. Open a command prompt/console/terminal. Navigate to where the `.pem` file is stored.
1. Connect to the instance with SSH. You can see more instructions and an example command by clicking Connect on the instance summary page and going to the SSH client tab.
1. Download the server to the instance by running the following command: `wget https://github.com/highrow623/pseudoregalia-multiplayer/releases/download/v0.2.0/pm-server-x86_64-unknown-linux-gnu`.
1. Add execute permissions to the server with the following command: `chmod +x pm-server-x86_64-unknown-linux-gnu`.
1. Run the server. For example, if you chose port 23432 to run the server on, run the following command: `./pm-server-x86_64-unknown-linux-gnu 0.0.0.0:23432`.

And now the server is up and running! The instance summary page has a Public IPv4 address and a Public DNS, either of which can be used in `settings.toml` for the `server.address` field.

### Clean Up the Instance

When you're done using the server, clean up the instance to help reduce costs.

1. [Optional] End the server process with crtl-C/cmd-C or by entering `/exit`.
1. On the instance summary page, click Instance state then Terminate (delete) instance.
