#if defined(WINDOWS) || defined(_WINDOWS) || defined(_WIN64) || defined(_WIN32)
# if defined(_M_AMD64) && !defined(_AMD64_)
#  define _AMD64_
# endif
#endif

#include <cstdint>
#include <tuple>
#include <utility>
#include <span>

#include "DeviceRegistry.hh"

using std::tuple;
using std::get;
using std::span;
namespace ranges = std::ranges;

// See envytools documentation at:
// https://envytools.readthedocs.io/en/latest/hw/pciid.html#introduction
static constexpr tuple<UINT16, UINT16> const
    PCI_ID_RANGE_TU102 { UINT16 { 0x1E00u }, UINT16 { 0x1E7Fu } },
    PCI_ID_RANGE_TU104 { UINT16 { 0x1E80u }, UINT16 { 0x1EFFu } },
    PCI_ID_RANGE_TU106 { UINT16 { 0x1F00u }, UINT16 { 0x1F7Fu } },
    PCI_ID_RANGE_TU116 { UINT16 { 0x2180u }, UINT16 { 0x21FFu } },
    PCI_ID_RANGE_TU117 { UINT16 { 0x1F80u }, UINT16 { 0x1FFFu } };

// See:
//      - https://admin.pci-ids.ucw.cz/read/PC/10de
//      - https://www.techpowerup.com/gpu-specs/?architecture=Turing&sort=name
//
static UINT16 Turing_Device_List_Skip[]
{
        // Tesla GPUs have some virtual memory with large BARs
        // Some Quadro GPUs already have resizable BAR

        UINT16_C(0x1E30),               // TU102GL [Quadro RTX 6000/8000] 24GB / 48GB
        UINT16_C(0x1E36),               // TU102GL [Quadro RTX 6000]	24GB
        UINT16_C(0x1E37),               // TU102GL [Tesla T10 16GB / GRID RTX T10-2/T10-4/T10-8]
        UINT16_C(0x1E38),               // TU102GL [Tesla T40 24GB]
        UINT16_C(0x1E3C),               // TU102GL
        UINT16_C(0x1E3D),               // TU102GL
        UINT16_C(0x1E3E),               // TU102GL
        UINT16_C(0x1E78),               // TU102GL [Quadro RTX 6000/8000] 24GB / 48GB

        UINT16_C(0x1EB9),               // TU104GL [T4 32GB]
        UINT16_C(0x1EBA),               // TU104GL [PG189 SKU600]
        UINT16_C(0x1EBE),               // TU104GL
},
        Turing_Device_List_2GB[] =
{
        UINT16_C(0x1F97),               // TU117M [GeForce MX450] 2GB
        UINT16_C(0x1F98),               // TU117M [GeForce MX450] 2GB
        UINT16_C(0x1F9C),               // TU117M [GeForce MX450] 2GB
        UINT16_C(0x1F9F),               // TU117M [GeForce MX550] 2GB
        UINT16_C(0x1FA0),               // TU117M [GeForce MX550] 2GB
},
        Turing_Device_List_4GB[] =
{
        UINT16_C(0x1F0A),               // TU106 [GeForce GTX 1650] 4GB

        // UINT16_C(0x1F81),               // TU117
        UINT16_C(0x1F82),               // TU117 [GeForce GTX 1650] 4GB
        UINT16_C(0x1F83),               // TU117 [GeForce GTX 1630] 4GB
        UINT16_C(0x1F91),               // TU117M [GeForce GTX 1650 Mobile / Max-Q] 4GB
        UINT16_C(0x1F92),               // TU117M [GeForce GTX 1650 Mobile] 4GB
        UINT16_C(0x1F94),               // TU117M [GeForce GTX 1650 Mobile] 4GB
        UINT16_C(0x1F95),               // TU117M [GeForce GTX 1650 Ti Mobile] 4GB
        UINT16_C(0x1F96),               // TU117M [GeForce GTX 1650 Mobile / Max-Q] 4GB
        UINT16_C(0x1F99),               // TU117M [GeForce GTX 1650 Mobile / Max-Q] 4GB
        UINT16_C(0x1F9D),               // TU117M [GeForce GTX 1650 Mobile / Max-Q] 4GB
        //UINT16_C(0x1F9E),
        //UINT16_C(0x1FA1),               // TU117M

        //UINT16_C(0x1FAE),               // TU117GL
        UINT16_C(0x1FB0),               // TU117GLM [Quadro T1000 Mobile] 4GB
        UINT16_C(0x1FB1),               // TU117GL [T600] 4GB
        UINT16_C(0x1FB2),               // TU117GLM [Quadro T400 Mobile] 4GB ??
        UINT16_C(0x1FB6),               // TU117GLM [T600 Laptop GPU] 4GB
        UINT16_C(0x1FB7),               // TU117GLM [T550 Laptop GPU] 4GB
        UINT16_C(0x1FB8),               // TU117GLM [Quadro T2000 Mobile / Max-Q] 4GB
        UINT16_C(0x1FB9),               // TU117GLM [Quadro T1000 Mobile] 4GB
        UINT16_C(0x1FBa),               // TU117GLM [T600 Mobile] 4GB
        UINT16_C(0x1FBB),               // TU117GLM [Quadro T500 Mobile] 4GB
        UINT16_C(0x1FBC),               // TU117GLM [T1200 Laptop GPU] 4GB
        //UINT16_C(0x1FBF),               // TU117GL

        UINT16_C(0x1FD9),               // TU117BM [GeForce GTX 1650 Mobile Refresh] 4GB
        UINT16_C(0x1FDD),               // TU117BM [GeForce GTX 1650 Mobile Refresh] 4GB

        UINT16_C(0x1FF2),               // TU117GL [T400 4GB]
        UINT16_C(0x1FF9),               // TU117GLM [Quadro T1000 Mobile] 4GB

        UINT16_C(0x2187),               // TU116 [GeForce GTX 1650 SUPER] 4GB
        UINT16_C(0x2188),               // TU116 [GeForce GTX 1650] 4GB
        UINT16_C(0x2192),	        // TU116M [GeForce GTX 1650 Ti Mobile] 4GB
},
        Turing_Device_List_8GB[] =
{
        UINT16_C(0x1E81),               // TU104 [GeForce RTX 2080 SUPER] 8GB
        UINT16_C(0x1E82),               // TU104 [GeForce RTX 2080] 8GB
        UINT16_C(0x1E84),               // TU104 [GeForce RTX 2070 SUPER] 8GB
        UINT16_C(0x1E87),               // TU104 [GeForce RTX 2080 Rev. A] 8GB
        UINT16_C(0x1E89),               // TU104 [GeForce RTX 2060] 6GB
        UINT16_C(0x1E90),               // TU104M [GeForce RTX 2080 Mobile] 8GB
        UINT16_C(0x1E91),               // TU104M [GeForce RTX 2070 SUPER Mobile / Max-Q] 8GB
        UINT16_C(0x1E93),               // TU104M [GeForce RTX 2080 SUPER Mobile / Max-Q] 8GB
        UINT16_C(0x1EAB),               // TU104M [GeForce RTX 2080 Mobile] 8GB
        UINT16_C(0x1EAE),               // TU104M [GeForce GTX 2080 Engineering Sample] 8GB ???
        UINT16_C(0x1EB1),               // TU104GL [Quadro RTX 4000]  8GB
        UINT16_C(0x1EB6),               // TU104GLM [Quadro RTX 4000 Mobile / Max-Q] 8GB

        UINT16_C(0x1EC2),               // TU104 [GeForce RTX 2070 SUPER] 8GB
        UINT16_C(0x1EC7),               // TU104 [GeForce RTX 2070 SUPER] 8GB
        UINT16_C(0x1ED0),               // TU104BM [GeForce RTX 2080 Mobile] 8GB
        UINT16_C(0x1ED1),               // TU104BM [GeForce RTX 2070 SUPER Mobile / Max-Q] 8GB
        UINT16_C(0x1ED3),               // TU104BM [GeForce RTX 2080 SUPER Mobile / Max-Q] 8GB

        UINT16_C(0x1F02),               // TU106 [GeForce RTX 2070] 8GB
        //UINT16_C(0x1F04),               // TU106

        UINT16_C(0x1F06),               // TU106 [GeForce RTX 2060 SUPER] 8GB
        UINT16_C(0x1F07),               // TU106 [GeForce RTX 2070 Rev. A] 8GB
        UINT16_C(0x1F08),               // TU106 [GeForce RTX 2060 Rev. A] 6GB
        UINT16_C(0x1F09),               // TU106 [GeForce GTX 1660 SUPER] 6GB
        UINT16_C(0x1F0B),               // TU106 [CMP 40HX] 8GB
        UINT16_C(0x1F10),               // TU106M [GeForce RTX 2070 Mobile] 8GB
        UINT16_C(0x1F11),               // TU106M [GeForce RTX 2060 Mobile] 6GB
        UINT16_C(0x1F12),               // TU106M [GeForce RTX 2060 Max-Q] 6GB
        UINT16_C(0x1F14),               // TU106M [GeForce RTX 2070 Mobile / Max-Q Refresh] 8GB
        UINT16_C(0x1F15),               // TU106M [GeForce RTX 2060 Mobile] 6GB
        //UINT16_C(0x1F2E),               // TU106M ??

        UINT16_C(0x1F36),               // TU106GLM [Quadro RTX 3000 Mobile / Max-Q] 6GB

        UINT16_C(0x1F42),               // TU106 [GeForce RTX 2060 SUPER]  8GB
        UINT16_C(0x1F47),               // TU106 [GeForce RTX 2060 SUPER]  8gb
        UINT16_C(0x1F50),               // TU106BM [GeForce RTX 2070 Mobile / Max-Q] 8GB
        UINT16_C(0x1F51),               // TU106BM [GeForce RTX 2060 Mobile] 6GB
        UINT16_C(0x1F54),               // TU106BM [GeForce RTX 2070 Mobile] 8GB
        UINT16_C(0x1F55),               // TU106BM [GeForce RTX 2060 Mobile] 6GB

        UINT16_C(0x1F76),               // TU106GLM [Quadro RTX 3000 Mobile Refresh] 6GB
        UINT16_C(0x1FF0),               // TU117GL [T1000 8GB]
        UINT16_C(0x2189),	        // TU116 [CMP 30HX] 6GB
        UINT16_C(0x2191),	        // TU116M [GeForce GTX 1660 Ti Mobile] 6GB

        UINT16_C(0x2182),                // TU116 [GeForce GTX 1660 Ti] 6GB
        UINT16_C(0x2183),                // TU116 [GeForce GTX 1660 Ti 8GB] 8GB
        UINT16_C(0x2184),                // TU116 [GeForce GTX 1660] 6GB
        // UINT16_C(0x21AE),             // TU116GL
        // UINT16_C(0x21BF),             // TU116GL
        // UINT16_C(0x21C2),             // TU116
},
        Turing_Device_List_16GB[] =
{
        UINT16_C(0x1E03),               // TU102 [GeForce RTX 2080 Ti 12GB]
        UINT16_C(0X1E04),               // TU102 [GeForce RTX 2080 Ti] 11GB
        UINT16_C(0x1E07),               // TU102 [GeForce RTX 2080 Ti Rev. A] 11GB
        UINT16_C(0x1E09),               // TU102 [CMP 50HX] 10GB
        UINT16_C(0x1E2D),               // TU102 [GeForce RTX 2080 Ti Engineering Sample] 11GB ???
        UINT16_C(0x1E2E),               // TU102 [GeForce RTX 2080 Ti 12GB Engineering Sample]

        UINT16_C(0x1EB0),               // TU104GL [Quadro RTX 5000] 16GB
        UINT16_C(0x1EB4),               // TU104GL [Tesla T4G] 16GB
        UINT16_C(0x1EB5),               // TU104GLM [Quadro RTX 5000 Mobile / Max-Q] 16GB
        UINT16_C(0x1EB8),               // TU104GL [Tesla T4] 16GB
        UINT16_C(0x1EF5),               // TU104GLM [Quadro RTX 5000 Mobile Refresh] 16GB

        UINT16_C(0x1F03)                // TU106 [GeForce RTX 2060 12GB] 12GB
},

        Turing_Device_List_32GB[] =
{
        UINT16_C(0x1E02),               // TU102 [Titan RTX] 24GB
};

static inline bool inRange(UINT16 value, tuple<UINT16, UINT16> const &range)
{
    return get<0u>(range) <= value && value <= get<1u>(range);
}

static inline bool isTU102(UINT16 deviceID)
{
    return inRange(deviceID, PCI_ID_RANGE_TU102);
}

static inline bool isTU104(UINT16 deviceID)
{
    return inRange(deviceID, PCI_ID_RANGE_TU104);
}

static inline bool isTU106(UINT16 deviceID)
{
    return inRange(deviceID, PCI_ID_RANGE_TU106);
}

static inline bool isTU116(UINT16 deviceID)
{
    return inRange(deviceID, PCI_ID_RANGE_TU116);
}

static inline bool isTU117(UINT16 deviceID)
{
    return inRange(deviceID, PCI_ID_RANGE_TU117);
}

bool isTuringGPU(UINT16 deviceID)
{
    return isTU102(deviceID) || isTU104(deviceID) || isTU106(deviceID) || isTU116(deviceID) || isTU117(deviceID);
}

static constexpr struct
{
    BarSizeSelector barSize;
    span<UINT16>    values;
}
    const DeviceRegistry[] =
{
    BarSizeSelector::Excluded, span { Turing_Device_List_Skip },
    BarSizeSelector:: _2G,     span { Turing_Device_List_2GB  },
    BarSizeSelector:: _4G,     span { Turing_Device_List_4GB  },
    BarSizeSelector:: _8G,     span { Turing_Device_List_8GB  },
    BarSizeSelector::_16G,     span { Turing_Device_List_16GB },
    BarSizeSelector::_32G,     span { Turing_Device_List_32GB }
};

BarSizeSelector lookupBarSizeInRegistry(UINT16 deviceID)
{
    for (auto const &group: DeviceRegistry)
        for (auto value: group.values)
            if (value == deviceID)
                return group.barSize;

    return BarSizeSelector::None;
}
