#!/usr/bin/env bash
set -euo pipefail

input_dir="${1:-../raw_demo}"
output_dir="${2:-assets/demo}"
ffmpeg_bin="${FFMPEG_BIN:-ffmpeg}"

environment_clip="$input_dir/仿真环境+模型展示.mp4"
before_clip="$input_dir/训练前展示.mp4"
during_clip="$input_dir/训练中展示.mp4"
after_clip="$input_dir/训练后展示.mp4"

for clip in "$environment_clip" "$before_clip" "$during_clip" "$after_clip"; do
  if [[ ! -f "$clip" ]]; then
    echo "Missing input clip: $clip" >&2
    exit 1
  fi
done

mkdir -p "$output_dir"

font="/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
bold_font="/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"

"$ffmpeg_bin" -y \
  -ss 8 -t 12 -i "$environment_clip" \
  -ss 8 -t 18 -i "$before_clip" \
  -ss 15 -t 18 -i "$during_clip" \
  -ss 43 -t 22 -i "$after_clip" \
  -f lavfi -t 4 -i color=c=0x081C2C:s=1280x720:r=30 \
  -f lavfi -t 4 -i color=c=0x081C2C:s=1280x720:r=30 \
  -filter_complex "\
    [4:v]drawtext=fontfile=${bold_font}:text='Learning-based Autonomous Navigation':fontcolor=white:fontsize=46:x=(w-text_w)/2:y=250,\
         drawtext=fontfile=${font}:text='Gazebo simulation | portfolio demo':fontcolor=0x63E6BE:fontsize=28:x=(w-text_w)/2:y=330[t];\
    [0:v]scale=1280:720:force_original_aspect_ratio=decrease,pad=1280:720:(ow-iw)/2:(oh-ih)/2:black,\
         drawbox=x=0:y=0:w=iw:h=82:color=black@0.72:t=fill,\
         drawtext=fontfile=${bold_font}:text='1  Environment and observation space':fontcolor=white:fontsize=30:x=34:y=25[e];\
    [1:v]scale=1280:720:force_original_aspect_ratio=decrease,pad=1280:720:(ow-iw)/2:(oh-ih)/2:black,\
         drawbox=x=0:y=0:w=iw:h=82:color=black@0.72:t=fill,\
         drawtext=fontfile=${bold_font}:text='2  Before training | baseline behaviour':fontcolor=white:fontsize=30:x=34:y=25[b];\
    [2:v]scale=1280:720:force_original_aspect_ratio=decrease,pad=1280:720:(ow-iw)/2:(oh-ih)/2:black,\
         drawbox=x=0:y=0:w=iw:h=82:color=black@0.72:t=fill,\
         drawtext=fontfile=${bold_font}:text='3  During training | policy optimisation':fontcolor=white:fontsize=30:x=34:y=25[d];\
    [3:v]scale=1280:720:force_original_aspect_ratio=decrease,pad=1280:720:(ow-iw)/2:(oh-ih)/2:black,\
         drawbox=x=0:y=0:w=iw:h=82:color=black@0.72:t=fill,\
         drawtext=fontfile=${bold_font}:text='4  After training | recorded evaluation':fontcolor=white:fontsize=30:x=34:y=25[a];\
    [5:v]drawtext=fontfile=${bold_font}:text='Evidence boundary':fontcolor=0x63E6BE:fontsize=42:x=(w-text_w)/2:y=230,\
         drawtext=fontfile=${font}:text='Learning policy\: simulation only':fontcolor=white:fontsize=29:x=(w-text_w)/2:y=310,\
         drawtext=fontfile=${font}:text='BPX platform\: ROS 2 driver + localisation + SLAM + Nav2':fontcolor=white:fontsize=27:x=(w-text_w)/2:y=360[z];\
    [t][e][b][d][a][z]concat=n=6:v=1:a=0,format=yuv420p[outv]" \
  -map "[outv]" -an -r 30 -c:v libx264 -preset medium -crf 23 -movflags +faststart \
  "$output_dir/autonomous_navigation_portfolio_demo.mp4"

"$ffmpeg_bin" -y -ss 58 \
  -i "$output_dir/autonomous_navigation_portfolio_demo.mp4" \
  -frames:v 1 -vf "scale=1280:720" -q:v 2 "$output_dir/demo_cover.jpg"

echo "Created $output_dir/autonomous_navigation_portfolio_demo.mp4"
echo "Created $output_dir/demo_cover.jpg"
