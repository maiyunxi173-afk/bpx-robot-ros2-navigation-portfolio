# Demo Notes

## Edited video

The application demo is available at [assets/demo/autonomous_navigation_portfolio_demo.mp4](../assets/demo/autonomous_navigation_portfolio_demo.mp4).

Approximate timeline:

| Time | Segment | What to look for |
| --- | --- | --- |
| 00:00-00:04 | Title and scope | The clip is explicitly labelled as a Gazebo learning-navigation experiment. |
| 00:04-00:16 | Environment and observations | Obstacle layout and simulated LiDAR perception used by the navigation experiment. |
| 00:16-00:34 | Before training | Baseline behaviour before policy convergence. |
| 00:34-00:52 | During training | Simulator and trajectory view while the policy is still being optimised. |
| 00:52-01:14 | After training | A recorded evaluation run showing more purposeful motion through the environment. |
| 01:14-01:18 | Evidence boundary | Simulation-only statement and pointers to the BPX ROS 2 stack. |

## Scope

This video supports a related learning-navigation experiment described in my application portfolio. It does **not** demonstrate deployment of the learned policy on the BPX quadruped. The physical-platform material in this repository is the ROS 2 driver/localisation/SLAM/Nav2 integration; the learning-navigation recording is included to show a complementary simulation workflow.

The four raw screen recordings contain no audible content. The edited version removes the silent audio tracks, shortens inactive desktop sections, standardises the output to 1280×720 H.264, and adds English captions for an admissions audience.

## Rebuilding the edit

The edit can be regenerated with FFmpeg:

```bash
bash scripts/build_portfolio_demo.sh /path/to/raw_clips
```

The input directory must contain:

- `仿真环境+模型展示.mp4`
- `训练前展示.mp4`
- `训练中展示.mp4`
- `训练后展示.mp4`

No quantitative success-rate or sim-to-real claim should be inferred from the video alone. A future evaluation release should add fixed seeds, episode counts, collision rate, success rate, path efficiency and the corresponding training configuration.
