# Scripts directory

This directory contains helper scripts (for Linux) that can be used to:
- Kick off make-based compilation, clean up
- Running application (.elf) files
- Running the Katana Visualizer

## Step 1: Running the Kt App IPC executable

Preferably, open a separate SSH terminal / shell, and run the following script:
```
./run_make_clean.sh
./run_kt_app_ipc.sh
```

This runs the Kt App IPC server; Katana Visualizer connects to this server to obtain metadata from KtLib.

## Running the Katana Visualizer

Preferably, in a separate SSH terminal / shell, launch the Visualizer thus:
```
./run_kt_viz.sh
```

The Visualizer opens up a web server on port 5000, and the user can point their web browser at the appropriate IP to view the web application.

Examples of URLs for viewing the visualizer:

- From PC / lpatop connected to RPi-5 via ethernet
``` 
http://10.10.1.1:5000
```
- From RPi-5 VNC itself
```
http://localhost:5000
```