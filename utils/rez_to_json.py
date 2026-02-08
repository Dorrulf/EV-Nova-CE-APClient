import json
import os
import subprocess

def run_executable(executable_name, input_file, converted_input_file_name):
    """Run the specified executable with the given input file."""
    try:
        subprocess.run([executable_name, "-totxt", input_file, converted_input_file_name], check=True)
    except FileNotFoundError:
        print(f"Error: '{executable_name}' not found in the current directory.")
        exit(1)
    except subprocess.CalledProcessError:
        print(f"Error: '{executable_name}' failed to run.")
        exit(1)
        
def cleanse_header(headerval):
    return headerval.replace(" ","_").replace('"','').replace('-','_').replace('.','').lower()

def add_name_dict(headers):
    nameDict = {cleanse_header(headers[i]): headers[i] for i in range(len(headers))}
    return nameDict
    
def add_struct_dict(headers):
    structDict = {cleanse_header(headers[i]): 'str' for i in range(len(headers))}
    return structDict

def convert_tsv_to_json(input_file, output_file):
    """Convert tab-delimited data to JSON, grouped by the first column."""
    try:
        with open(input_file, 'r') as tsv_file:
            json_data = {}
            json_data["names"] = []
            json_data["structs"] = []

            # Read each line in the TSV file
            i = 1   # so, yeah, not gonna 0 index here, but we could... Notepad++ like to start at 1 in a text file.
            readHeaders = True
            debug_val_block = ""
            for line in tsv_file:
                if (i < 5):
                    i = i + 1
                    continue # EVN puts file info in the first 4 lines, which we don't need.
                    
                # The data file may have multiple data types.
                # When it does, each type is output in its own table with a blank line between each
                # So when we get to the first line of a new table, we need to read the new headers
                if (readHeaders):
                    headers = line.strip().replace('"', '').split('\t')   
                    readHeaders = False
                    print(f"reading new headers: {headers[0]}")
                    
                    json_data["names"].append(add_name_dict(headers))
                    json_data["structs"].append(add_struct_dict(headers))
                    continue
                
                values = line.strip().replace('"', '').split('\t')
                
                # detect a new line, marking the end of the previous table
                # and prime for the next table.
                # Skip the blank line to avoid bad values and indexing issues.
                if (values[0] is None or values[0] == ""):
                    readHeaders = True
                    print(f"done reading {debug_val_block}")
                    continue
                else:
                    debug_val_block = values[0]
                
                key = values[0]  # Group by the first column
                #row_data = {values[1]: {headers[i]: values[i] for i in range(len(headers))}}
                row_data = {values[1]: {cleanse_header(headers[i]): values[i] for i in range(len(headers))}}

                # Append to the appropriate group
                if key not in json_data:
                    json_data[key] = []
                json_data[key].append(row_data)

        # Write the grouped JSON data to the output file
        with open(output_file, 'w') as json_file:
            json.dump(json_data, json_file, indent=4)

        print(f"Successfully converted '{input_file}' to '{output_file}'")

    except Exception as e:
        print(f"An error occurred: {e}")

def main():
    # Define executable name and input/output files
    executable_name = 'EVNEW.exe'  # Replace with your executable name
    input_file_name = 'Nova Data 1.rez'  # Input file for the executable
    converted_input_file_name = 'data.txt'  # give a name to the file EVNEW will produce, that will be given to the json converter
    output_file_name = 'data.json'  # Desired output JSON file

    # Check if the executable exists in the current directory
    if not os.path.isfile(executable_name):
        print(f"Error: '{executable_name}' not found in the current directory.")
        return

    # Run the executable
    run_executable(executable_name, input_file_name, converted_input_file_name)

    # Convert the output file to JSON
    convert_tsv_to_json(converted_input_file_name, output_file_name)

if __name__ == "__main__":
    main()
