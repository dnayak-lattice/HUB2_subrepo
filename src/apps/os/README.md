# OS specific functions

This directory contains the source code and headers for OS-specific functions, dependencies, etc that are requried by HUB / KtLib / Kt App. 

It also contains the DAL (Device Abstraction Layer) functions - these are functions that perform operations such as read/write, etc. on the underlying bus interfaces, on behalf of the HUB / Ktlib layers.

They are expected to be implemented by the Application.

The current release contains support for Linux.

