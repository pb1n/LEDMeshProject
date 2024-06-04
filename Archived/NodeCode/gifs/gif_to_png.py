import os
from PIL import Image
import sys

def scale_gif_and_convert_frames(input_gif_path, output_size=(16, 16)):
    # Open the GIF file
    with Image.open(input_gif_path) as gif:
        # Get the base name of the gif without extension
        gif_name = os.path.splitext(os.path.basename(input_gif_path))[0]
        
        # Create the new directory path
        root_dir = os.path.dirname(input_gif_path)
        output_dir = os.path.join(root_dir, '..', 'data', 'images', gif_name)
        os.makedirs(output_dir, exist_ok=True)
        
        # Iterate through each frame
        frame_number = 0
        try:
            while True:
                # Seek to the current frame
                gif.seek(frame_number)
                
                # Scale the frame to the desired size
                frame = gif.resize(output_size, Image.LANCZOS)
                
                # Save the frame as a PNG file
                frame.save(os.path.join(output_dir, f"{gif_name}-{frame_number}.png"), "PNG")
                
                # Move to the next frame
                frame_number += 1
        except EOFError:
            # This exception is raised when there are no more frames in the GIF
            pass

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python scale_gif.py <path_to_gif>")
        sys.exit(1)
    
    input_gif_path = sys.argv[1]
    scale_gif_and_convert_frames(input_gif_path)
    print(f"Frames have been saved to directory: data/images/{os.path.splitext(os.path.basename(input_gif_path))[0]}")
