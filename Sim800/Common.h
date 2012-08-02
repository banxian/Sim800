#ifndef _COMMON_H_
#define _COMMON_H_

enum TLogType { 
    ltHint, ltDebug, ltMessage, ltError
};

enum TStreamCodec {
    scUnknown = -1,
    scStored = 0,
    scSHRUNK,
    scReduced1,
    scReduced2,
    scReduced3,
    scReduced4,
    scImploded,
    scToken,
    scDeflate,
    scDeflate64
};

enum ResizeMode {
    rmSafe,
    rmPixelToPixel,
    rmFullScreen,
    rmIPad,
    rmNewIPad,
    rmIPhone,
    rmRetinaIPhone
};

enum CodecMode {
    cmPNG,
    cmJPEGVeryHigh,
    cmJPEGMedium
};

enum FilterMode {
    fmNone,
    fmSlight,
    fmHigh,
    fmMaximum
};

enum PageSortingMethod {
    psmAutomatic,
    psmAlpha,
    psmNone
};

#endif
