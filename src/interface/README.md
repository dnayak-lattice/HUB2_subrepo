# Interface
This directory contains header files that serve as interfaces between the various project components, viz. HUB, KTLIB, GARD(FW), and APP.

Each component in turn should refer to only the header files in this directory to access features and information about any other component.

Presently, we have the following component-wise header files:

- dal.h (Device adapatation layer)
- hub.h (HUB library and data movement engine)
- Kt_Lib.h (Katana library)
- gard_hub_iface.h (Contract of communications between HUB and GARD FW)
- hmi_metadata_face.h (HMI related Host-App layer to GARD App-module layer contract)
