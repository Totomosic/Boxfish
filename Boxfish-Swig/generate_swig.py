import os
import argparse

INTERFACE_FILE = "./Boxfish.i"

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("output_directory", type=str, help="Output directory")
    parser.add_argument("--swig", type=str, required=True, help="Path to swig executable")
    args = parser.parse_args()

    os.makedirs(args.output_directory, exist_ok=True)

    os.system("\"{}\" -c++ -python -outdir {} -o {} {}".format(args.swig, args.output_directory, "Boxfish_wrapper.cpp", INTERFACE_FILE))
