"""
Generates a build of the game.
"""

import os
import sys
import shutil
import subprocess
import zipfile
import json
import argparse

ENGINE_ASSET_DIR = "./third_party/vultron/Vultron/assets/"
SHADER_EXT = ".spv"
ENGINE_FILES = ["brdf.dat", "skybox.dat"]
ASSET_DIR = "./assets"
ASSET_EXT = [".dat", ".bank"]
GAME_NAME = "monke"
VERSION_FILE = "./VERSION"

def copy_assets(output_dir):
    # Copy assets
    for root, _, files in os.walk(ASSET_DIR):
        rel_path = os.path.relpath(root, ASSET_DIR)
        target_dir = os.path.join(output_dir, rel_path)
    
        os.makedirs(target_dir, exist_ok=True)
        
        for file in files:
            if os.path.splitext(file)[1].lower() in ASSET_EXT:
                src_file_path = os.path.join(root, file)
                target_file_path = os.path.join(target_dir, file)            
                shutil.copy(src_file_path, target_file_path)
    
    # Copy engine assets
    for root, _, files in os.walk(ENGINE_ASSET_DIR):
        for file in files:
            if os.path.splitext(file)[1].lower() == SHADER_EXT or file in ENGINE_FILES:                
                # Get path relative to the engine asset directory
                rel_path = os.path.relpath(root, ENGINE_ASSET_DIR)
                os.makedirs(os.path.join(output_dir, rel_path), exist_ok=True)
                src = os.path.join(root, file)
                dst = os.path.join(output_dir, rel_path, file)
                shutil.copy(src, dst)
                print(f"Copying {src} to {dst}")

                

def build_game(output_dir, version, config="Release"):
    build_path = os.path.join(output_dir, "build")
    os.makedirs(build_path, exist_ok=True)

    # Generate the build files, release flag last
    subprocess.run(["cmake", "-S", "./", "-B", build_path, "-DMK_ASSET_DIR=./assets", "-DVLT_ASSET_DIR=./assets", "-DMK_DIST=ON"])
    
    # Build the game
    subprocess.run(["cmake", "--build", build_path, "--config", config, "--target", GAME_NAME, "-j", "32"])

    # Copy the game executable
    shutil.copy(os.path.join(build_path, config, f"{GAME_NAME}_v{version}.exe"), os.path.join(output_dir, GAME_NAME + ".exe"))

    # Copy dlls in the build directory to the output directory
    for root, _, files in os.walk(os.path.join(build_path, config)):
        for file in files:
            if os.path.splitext(file)[1].lower() == ".dll":
                src = os.path.join(root, file)
                dst = os.path.join(output_dir, file)
                shutil.copy(src, dst)
                print(f"Copying {src} to {dst}")

    # Remove the build directory
    if input("Do you want to remove the build directory? (y/n): ").lower() == "y":
        shutil.rmtree(build_path)
        
def main():
    parser = argparse.ArgumentParser(description="Generate a build of the game")
    parser.add_argument("-o", "--output", help="The output directory", default=f"dist/{GAME_NAME}")
    parser.add_argument("-b", "--build", help="Build the game executable and content", action="store_true")
    # We want to be able to only upload
    parser.add_argument("--steam", help="Upload the build to Steam", action="store_true")
    parser.add_argument("--platform", help="The target platform", choices=["windows", "linux", "macos"], default="windows")

    args = parser.parse_args()

    if os.path.exists(args.output) and args.build:
        shutil.rmtree(args.output)

    if not os.path.exists(args.output):
        os.makedirs(args.output)

    with open(VERSION_FILE, "r") as f:
        version = f.read().strip()

    if args.build:
        print("Copying assets...")
        asset_path = os.path.join(args.output, "assets")
        os.makedirs(asset_path, exist_ok=True)
        copy_assets(asset_path)

    if args.build:
        print("Building game...")
        build_game(args.output, version)

if __name__ == "__main__":
    main()
    



    