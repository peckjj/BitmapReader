// ====================================================================
// Define Bitmap Signature Types

typedef uint16_t BitMapSigType;

#define SIGT_BM (BitMapSigType)0x4d42
#define SIGT_BA (BitMapSigType)0x4142
#define SIGT_CI (BitMapSigType)0x4943
#define SIGT_CP (BitMapSigType)0x5043
#define SIGT_IC (BitMapSigType)0x4349
#define SIGT_PT (BitMapSigType)0x5450

bool isSigTypeSupported(BitMapSigType sigType);

// ====================================================================
