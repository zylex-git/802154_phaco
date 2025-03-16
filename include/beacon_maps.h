#pragma once

#include <map>
#include <cstdint>

namespace beacon_maps {

    const std::map<uint8_t, const char*> network_state_map = {
        {0, "Network Idle"},
        {1, "Reserved"},
        {2, "Network Footswitch Active"},
        {3, "Network Pairing"}
    };

    const std::map<uint8_t, const char*> beacon_source_map = {
        {0, "Centurion Console"},
        {1, "Microscope"}
    };

    const std::map<uint8_t, const char*> step_type_map = {
        {0, "Startup"},
        {1, "Shutdown"},
        {2, "Setup"},
        {3, "I/A"},
        {4, "Phaco"},
        {5, "AutoSert (IOL Injection)"},
        {6, "Coagulation"},
        {7, "Vitrectomy"},
        {8, "Irrigation Footswitch"},
        {9, "Fill"},
        {10, "AutoCap (Capsulotomy)"},
        {11, "Service"},
        {12, "V+V (SGS)"},
        {13, "End of Case"}
    };

    const std::map<uint8_t, const char*> step_sub_type_map = {
        {0, "None"},
        {1, "PrePhaco"},
        {2, "Sculpt"},
        {3, "Quad"},
        {4, "Chop"},
        {5, "Epinucleus"},
        {6, "Flip"},
        {7, "UltraChop"},
        {8, "Cortex"},
        {9, "Polish"},
        {10, "Visco"},
        {11, "Anterior Vitrectomy"},
        {12, "Epinucleus Removal"},
        {13, "Peripheral Iridectomy"},
        {14, "Visco Aspiration"},
        {15, "I/A Cut"},
        {16, "Position"},
        {17, "Incision"},
        {18, "Centration"},
        {19, "Toric"},
        {20, "Registration"},
        {21, "Capsularhexis"},
        {22, "Pseudophakic"},
        {23, "Aphakic"},
        {24, "Axis Marker"},
        {25, "Cut I/A"}
    };
} // namespace beacon_maps