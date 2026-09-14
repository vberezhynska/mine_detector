Bio-Hybrid Mine Detection System (ROS 2)

A modular, ROS 2-based prototype for a bio-hybrid humanitarian demining support system. This project explores the integration of biological search agents (simulated or real field sensors) with autonomous data-filtering pipelines, real-time spatial mapping, and instant feedback mechanisms.

🛠 Project Architecture The system is structured as a decentralized ROS 2 workspace (mine_detector) consisting of three core packages:

rat_hardware_interface: Manages real-world hardware integration, parsing live GPS coordinates (sensor_msgs/NavSatFix) and digital touch sensor events (std_msgs/Bool).

rat_mapping_manager: The analytical core. It applies Bayesian probability filtering to eliminate single-touch false positives and utilizes spatial clustering to map accurate threat locations.

rat_feedback_controller: Manages the immediate, zero-latency feedback loop (auditory beep signal) to maintain operational conditioning during data collection.

🚀 Getting Started Prerequisites ROS 2 (Humble / Iron / Jazzy)

Python 3.x

Colcon build tools

Building the Workspace Clone the repository and build the workspace using colcon:

Bash mkdir -p ~/mine_detector/src cd ~/mine_detector/src

Clone or move your packages here
cd .. colcon build --symlink-install source install/setup.bash 📊 System Data Flow Plaintext [ Touch Sensor & GPS ] │ ├──► (Immediate Loop) ──► [ Feedback Beep Node ] (Instant audio confirmation) │ └──► (Analytical Loop) ──► [ Bayesian Filter ] ──► [ Spatial Clustering ] ──► [ RViz2 Map Marker ] 📜 License This project is developed for academic/course research purposes. Feel free to use and modify.