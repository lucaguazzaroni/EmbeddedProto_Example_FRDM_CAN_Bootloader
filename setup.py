#
# Copyright (C) 2020-2024 Embedded AMS B.V. - All Rights Reserved
#
# This file is part of Embedded Proto.
#
# Embedded Proto is open source software: you can redistribute it and/or 
# modify it under the terms of the GNU General Public License as published 
# by the Free Software Foundation, version 3 of the license.
#
# Embedded Proto  is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Embedded Proto. If not, see <https://www.gnu.org/licenses/>.
#
# For commercial and closed source application please visit:
# <https://EmbeddedProto.com/license/>.
#
# Embedded AMS B.V.
# Info:
#   info at EmbeddedProto dot com
#
# Postal address:
#   Atoomweg 2
#   1627 LE, Hoorn
#   the Netherlands
#

import subprocess
import argparse
import os
import venv
import platform
from sys import stderr
import shutil

# Perform a system call to beable to display colors on windows
os.system("")

CGREEN = '\33[92m'
CRED = '\33[91m'
CYELLOW = '\33[93m'
CEND = '\33[0m'

print("Update the submodule Embedded Proto before importing its setup script.", end='')
try:
    result = subprocess.run(["git", "submodule", "init"], check=False, capture_output=True)
    if result.returncode:
        print(" [Fail]")
        print(result.stderr.decode("utf-8"), end='', file=stderr)
        exit(1)
        
    result = subprocess.run(["git", "submodule", "update"], check=False, capture_output=True)
    if result.returncode:
        print(" [Fail]")
        print(result.stderr.decode("utf-8"), end='', file=stderr)
        exit(1)
    else:
        print(" [Success]")
except OSError:
    print(" [Fail]")
    print("Unable to find git in your path.")
    print("Stopping the setup.")
    exit(1)
except Exception as e:    
    print(" [Fail]")
    print("Error: " + str(e), file=stderr)
    exit(1)
        
from EmbeddedProto import setup as EPSetup


####################################################################################

def setup_desktop_script():
    try:
        # ---------------------------------------
        print("Setup the virtual environment for the desktop Python script.", end='')
        os.chdir("./desktop")
        venv.create("venv", with_pip=True)
        print(" [" + CGREEN + "Success" + CEND + "]")
        
        # ---------------------------------------
        print("Installing requirement for the desktop Python script.", end='')
        on_windows = "Windows" == platform.system()
        command = []
        if on_windows:
            command.append("./venv/Scripts/pip")
        else:
            command.append("./venv/bin/pip")
        command.extend(["install", "-r", "requirements.txt"])
        result = subprocess.run(command, check=False, capture_output=True)
        if result.returncode:
            print(" [" + CRED + "Fail" + CEND + "]")
            print(result.stderr.decode("utf-8"), end='', file=stderr)
            exit(1)
        else:
            print(" [" + CGREEN + "Success" + CEND + "]")
            
        os.chdir("..")
        
    except Exception as e:    
        print(" [" + CRED + "Fail" + CEND + "]")
        print("Error: " + str(e), file=stderr)
        exit(1)


####################################################################################

def generate_source_code():

    print("Generate the source file based on netbooting.proto.", end='')
    try:
        os.chdir("EmbeddedProto")

        command = ["protoc", "-I./generator", "--python_out=./generator/EmbeddedProto", 
                   "./generator/embedded_proto_options.proto"]
        result = subprocess.run(command, check=False, capture_output=True)
        if result.returncode:
            print(" [" + CRED + "Fail" + CEND + "]")
            print(result.stderr.decode("utf-8"), end='', file=stderr)
            exit(1)
        
        command = ["protoc", "-I../proto", "-I./generator/", "-I./generator/EmbeddedProto",
                   "--eams_out=../frdm-ke06z/generated", 
                   "../proto/netbooting.proto"]
        if "Windows" == platform.system():
            command.append("--plugin=protoc-gen-eams=protoc-gen-eams.bat")
        else:
            command.append("--plugin=protoc-gen-eams=protoc-gen-eams")
            
        result = subprocess.run(command, check=False, capture_output=True)
        if result.returncode:
            print(" [" + CRED + "Fail" + CEND + "]")
            print(result.stderr.decode("utf-8"), end='', file=stderr)
            exit(1)
            
        os.chdir("..")
            
        result = subprocess.run(["protoc", "-I./proto", "-I./EmbeddedProto/generator", "--python_out=./desktop/generated", 
                                 "./proto/netbooting.proto"], check=False, capture_output=True)
        if result.returncode:
            print(" [" + CRED + "Fail" + CEND + "]")
            print(result.stderr.decode("utf-8"), end='', file=stderr)
            exit(1)
        else:
            # WORKAROUND
            src_path = os.path.join(".", "EmbeddedProto", "generator", "EmbeddedProto", "embedded_proto_options_pb2.py")
            dest_path = os.path.join(".", "desktop", "generated", "embedded_proto_options_pb2.py")
            shutil.copy(src_path, dest_path)
            
            generated_path = os.path.join(".", "desktop", "generated", "netbooting_pb2.py")
            with open(generated_path, 'r', encoding='utf-8') as file:
                lines = file.readlines()

            with open(generated_path, 'w', encoding='utf-8') as file:
                for line in lines:
                    if "import embedded_proto_options_pb2" in line:
                        line = line.replace("import embedded_proto_options_pb2", "from . import embedded_proto_options_pb2")
                    file.write(line)
    
            print(" [" + CGREEN + "Success" + CEND + "]")                                 
                        
    except Exception as e:    
        print(" [" + CRED + "Fail" + CEND + "]")
        print("Error: " + str(e), file=stderr)
        exit(1)



####################################################################################

if __name__ == "__main__":

    parser = argparse.ArgumentParser(description="This script will setup the environment to generate Embedded Proto "
                                                 "code in your project.")

    parser.add_argument("-g", "--generate", action="store_true", 
                        help="Do not run the Embedded Proto setup. Only generate the source based on *.proto files. "
                             "Use this after changing netbooting.proto.")

    EPSetup.add_parser_arguments(parser)
    EPSetup.display_feedback()
    args = parser.parse_args()

    # ---------------------------------------
    # Create destination folders if not present.
    newpath = r"./frdm-ke06z/generated"
    if not os.path.exists(newpath):
        os.makedirs(newpath)
    newpath = r"./desktop/generated"
    if not os.path.exists(newpath):
        os.makedirs(newpath)
     

    # ---------------------------------------
    # Clean excisting venv and other generated files.
    if args.clean:
        shutil.rmtree("./desktop/venv", ignore_errors=True)

        try:
            os.remove("./desktop/generated/netbooting_pb2.py")
        except FileNotFoundError:
            # This exception we can safely ignore as it means the file was not there. In that case we do not have to remove
            # it.
            pass

        try:
            os.remove("./frdm-ke06z/generated/netbooting.h")
        except FileNotFoundError:
            # This exception we can safely ignore as it means the file was not there. In that case we do not have to remove
            # it.
            pass

    # ---------------------------------------
    if not args.generate:
        os.chdir("./EmbeddedProto")
        
        # Run the setup script of Embedded Proto it self.
        EPSetup.run(args)
        
        os.chdir("..")   
        
    # ---------------------------------------
    setup_desktop_script()

    # ---------------------------------------
    generate_source_code()

    # ---------------------------------------
    # If the script did not exit before here we have completed it with success.
    print("Setup completed with success!")