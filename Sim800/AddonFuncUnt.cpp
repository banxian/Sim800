#include "AddonFuncUnt.h"
//#include "reader/exifdefine.h"

#include <QtCore/QtGlobal>
#include <QtCore/QtEndian>
#include <QtCore/QCryptographicHash>
#include <QtGui/QPainter>
#include <zlib/zlib.h> // TODO: Qt wrapped need QtCore
#include <QtCore/QDir>
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACX)
#   include <unistd.h>
#elif defined(Q_OS_MACX)
#   include <mach/mach.h>
#   include <mach/machine.h>
#endif
#include <QtCore/QMap>
#include <QtGui/QApplication>


QImage NewQImageFromRCDATA(LPCTSTR lpName)
{
    QImage Result;
    LoadQImageFromResource(Result, GetModuleHandle(NULL), RT_RCDATA, lpName);
    return Result;
}

QImage NewQImageFromResource( HMODULE hModule, LPCTSTR lpType, LPCTSTR lpName )
{
    QImage Result;
    LoadQImageFromResource(Result, hModule, lpType, lpName);
    return Result;
}

bool LoadQImageFromResource(QImage& qimage, HMODULE hModule, LPCTSTR lpType, LPCTSTR lpName) {
    HRSRC resinfo = FindResource(hModule, lpName, lpType);
    HGLOBAL resdata = LoadResource(hModule, resinfo);
    DWORD ressize = SizeofResource(hModule, resinfo);
    LPTSTR resbyte = LPTSTR(LockResource(resdata));
    qimage.loadFromData((uchar*)resbyte, ressize);
    return resbyte != 0;
}

void Normalize8bitQImage( QImage &qimage )
{
    if (qimage.depth() != 8) {
        return;
    }
    if (qimage.colorTable().isEmpty() == false) {
        // 8bpp + color table, or 1bpp image
        // JPEG Grayscale (v1.1)
        qimage = qimage.convertToFormat(QImage::Format_RGB32);
        return;
    }
    // only older Qt version decode JPEG Grayscale without colortable
    const uchar *src = qimage.bits();
    int srcwidth = qimage.width();
    int srcheight = qimage.height();
    QImage viaimage = QImage(srcwidth, srcheight, QImage::Format_RGB32);
    quint32 *var = (quint32*)viaimage.bits();
    //const uchar *srcend = src + qimage.bytesPerLine() * srcheight;
    int padbytes = viaimage.bytesPerLine() - viaimage.width();
    for (int y = 0; y < srcheight; y++) {
        for (int x = 0; x < srcwidth; x++) {
            *var = *src | *src << 8 | *src << 16 | 0xFF000000;
            var++;
            src++;
        }
        var += padbytes;
    }

    qimage = viaimage;
}

void QuantizeToGrayscaleQImage( QImage& qimage, bool dither /* = false */ )
{
    if (qimage.depth() != 32) {
        if (qimage.depth() == 8 && IsGrayscaleImage(qimage)) {
            return;
        } else {
            Normalize8bitQImage(qimage);
        }
    }
    QImage viaimage = QImage(qimage.width(), qimage.height(), QImage::Format_Indexed8);
    QVector < QRgb > grayscale4;

    for (QRgb i = 0; i <= 0xFFFFFF; i += (dither?0x111111:0x010101))
    {
        grayscale4.append(i | 0xFF000000);
    }
    viaimage.setColorTable(grayscale4);

    // TODO: AutoLevel?

    const uchar *src_data = qimage.bits();
    uchar *dest_data = viaimage.bits();
    int srcwidth = qimage.width();
    int srcheight = qimage.height();

    if (dither) {
        int* line1;
        int* line2;
        int* pv;
        QScopedArrayPointer<int> lineBuffer(new int[srcwidth * 3]);
        line1 = lineBuffer.data();
        line2 = lineBuffer.data() + srcwidth;
        pv = lineBuffer.data() + srcwidth * 2;

        int endian = (QSysInfo::ByteOrder == QSysInfo::BigEndian);
        for (int y = 0; y < srcheight; y++) {
            const uchar* q = src_data;
            const uchar* q2 = y < srcheight - 1 ? q + qimage.bytesPerLine() : qimage.bits();
            uchar *b = dest_data;

            int *l1 = (y&1) ? line2 : line1;
            int *l2 = (y&1) ? line1 : line2;
            // Shrink to 16bit Grayscale
            if (y == 0) {
                for (int i = 0; i < srcwidth; i++) {
                    if (endian) {
                        l1[i] = qMin(65535, (q[i*4+3]*19595 + q[i*4+2]*38469 + q[i*4+1]*7472) >> 8);
                    } else {
                        l1[i] = qMin(65535, (q[i*4]*19595 + q[i*4+1]*38469 + q[i*4+2]*7472) >> 8);
                    }
                }
            }
            if (y + 1 < srcheight) {
                for (int i = 0; i < srcwidth; i++) {
                    if (endian) {
                        l2[i] = qMin(65535, (q2[i*4+3]*19595 + q2[i*4+2]*38469 + q2[i*4+1]*7472) >> 8);
                    } else {
                        l2[i] = qMin(65535, (q2[i*4]*19595 + q2[i*4+1]*38469 + q2[i*4+2]*7472) >> 8);
                    }
                }
            }
            // Bi-directional error diffusion
            if (y&1) {
                for (int x = 0; x < srcwidth; x++) {
                    int newpixel = qMax(qMin(15, (l1[x] * 15 + 32768) / 65536), 0); // 16bit to 4bit
                    int quant_error = l1[x] - newpixel * 65536 / 15; // 0~255 -> 0~65535
                    pv[x] = newpixel; // 4bit Grayscale

                    // Spread the error around...
                    if (x + 1 < srcwidth) {
                        l1[x+1] += (quant_error*7)>>4;
                        l2[x+1] += quant_error>>4;
                    }
                    l2[x]+=(quant_error*5)>>4;
                    if (x>1) {
                        l2[x-1]+=(quant_error*3)>>4;
                    }
                }
            } else {
                for (int x = srcwidth; x-- > 0;) {
                    int newpixel = qMax(qMin(15, (l1[x] * 15 + 32768)/ 65536), 0);
                    int quant_error = l1[x] - newpixel * 65536 / 15;
                    pv[x] = newpixel;

                    // Spread the error around...
                    if (x > 0) {
                        l1[x-1] += (quant_error*7)>>4;
                        l2[x-1] += quant_error>>4;
                    }
                    l2[x]+=(quant_error*5)>>4;
                    if (x + 1 < srcwidth) {
                        l2[x+1]+=(quant_error*3)>>4;
                    }
                }
            }

            for (int x = 0; x < srcwidth; x++) {
                *b++ = uchar(pv[x]);
            }
            src_data += qimage.bytesPerLine();
            dest_data += viaimage.bytesPerLine();
        }
    } else {
        int padbytes = (viaimage.bytesPerLine() - srcwidth);
        bool endian = (QSysInfo::ByteOrder == QSysInfo::BigEndian);
        if (endian) {
            // ABGR
            for (int y = 0; y < srcheight; y++) {
                for (int x = 0; x < srcwidth; x++) {
                    *dest_data = qMin(255, (src_data[3]*1224 + src_data[2]*2405 + src_data[1]*467) >> 12);
                    src_data += 4;
                    dest_data += 1;
                }
                dest_data += padbytes;
            }
        } else {
            // RGBA
            for (int y = 0; y < srcheight; y++) {
                for (int x = 0; x < srcwidth; x++) {
                    *dest_data = qMin(255, (src_data[0]*1224 + src_data[1]*2405 + src_data[2]*467) >> 12);
                    src_data += 4;
                    dest_data += 1;
                }
                dest_data += padbytes;
            }
        }
    }

    qimage = viaimage;
}

void AutoExposureImage( QImage& qimage, bool heavy /*= false */ )
{
    bool fromgrayscale = IsGrayscaleImage(qimage);
    bool is8bit = qimage.depth() == 8;
    if (is8bit && qimage.isGrayscale() == false) {
        Normalize8bitQImage(qimage);
    } else if (qimage.depth() != 32) {
        Normalize8bitQImage(qimage);
    }
    is8bit = qimage.depth() == 8; // and fromgrayscale

    uchar* src = qimage.bits();
    uchar* src_end = src + qimage.byteCount();
    QMap < quint16, quint32 > grayhistogram;
    int pixelcount = qimage.width() * qimage.height();

    if (is8bit) {
        // align
        for (int y = 0; y < qimage.height(); y++) {
            for (int x = 0; x < qimage.width(); x++) {
                if (grayhistogram.contains(*src)) {
                    grayhistogram[*src]++;
                } else {
                    grayhistogram.insert(*src, 1);
                }
                src++;
            }
            src += qimage.bytesPerLine() - qimage.width();
        }
    } else {
        while (src < src_end) {
            quint16 gray = (fromgrayscale?src[0]:qMin(65535, (src[0]*19595 + src[1]*38469 + src[2]*7472) >> 8));
            if (grayhistogram.contains(gray)) {
                grayhistogram[gray]++;
            } else {
                grayhistogram.insert(gray, 1);
            }
            src += 4;
        }
    }
    int absblack = 0, abswhite = (fromgrayscale?255:65280);
    if (grayhistogram.size() >= 2) {
        absblack = grayhistogram.constBegin().key();
        abswhite = (grayhistogram.constEnd() - 1).key();
    }
    qDebug("absolute black:%3d, absolute white:%3d", absblack, abswhite);
    if (absblack > 0 || abswhite < (is8bit?255:65280)) {
        // we can waste time
        int sum = 0, oppblack = 0, oppwhite = (fromgrayscale?255:65280);
        if (heavy) {
            int inc = 0, dec = 0, semiblack = absblack, semiwhite = abswhite;
            int topblackvalue = grayhistogram.constBegin().value();
            int topwhitevalue = (grayhistogram.constEnd() - 1).value();
            oppblack = absblack;
            oppwhite = abswhite;
            for (QMap < quint16, quint32 >::const_iterator it = grayhistogram.constBegin(); it != grayhistogram.constEnd(); it++) {
                // opposite black
                if (it.value() > topblackvalue) {
                    topblackvalue = it.value();
                    semiblack = it.key();
                    inc = 0; // reset
                    dec = 0; // reset
                }
                if (it.value() < topblackvalue) {
                    if (it != grayhistogram.constBegin() && it.value() < (it - 1).value()) {
                        dec += ((it - 1).value() - it.value()); // hole
                    }
                    if (it != grayhistogram.constBegin() && it.value() > (it - 1).value()) {
                        inc += (it.value() - (it - 1).value());
                    }
                    // distance
                    if (dec - inc != 0 && it.key() - semiblack != 0 && topblackvalue != 0) {
                        // prevent div 0 exception
                        if ((abswhite - absblack) / (it.key() - semiblack) >= 8 && (abswhite - absblack) / (it.key() - semiblack) <= 30 &&  double(it.value()) / topblackvalue <= 0.5) {
                            // distance 3%~13%, value limit 50% topvalue
                            oppblack = semiblack;
                        }
                    }
                }
                if (it.key() >= absblack + (abswhite - absblack) / 3 ) {
                    break; // leave oppblack untouch
                }
            }
            inc = 0; dec = 0;
            for (QMap < quint16, quint32 >::const_iterator it = grayhistogram.constEnd(); it != grayhistogram.constBegin(); ) {
                // opposite white
                it--;
                // ever gather or equal should be a reset signal in white limit detection
                if (it.value() >= topwhitevalue) {
                    topwhitevalue = it.value();
                    semiwhite = it.key();
                    inc = 0;
                    dec = 0;
                }
                if (it.value() < topwhitevalue) {
                    if (it != (grayhistogram.constEnd() - 1) && it.value() < (it + 1).value()) {
                        dec += ((it + 1).value() - it.value()); // hole
                    }
                    if (it != (grayhistogram.constBegin() - 1) && it.value() > (it + 1).value()) {
                        inc += (it.value() - (it + 1).value());
                    }
                    // dec - inc == (topwhitevalue - it.value())
                    if (dec - inc != 0 && semiwhite - it.key() != 0 && topwhitevalue != 0) {
                        if ((abswhite - absblack) / (semiwhite - it.key()) >= 7 && (abswhite - absblack) / (semiwhite - it.key()) <= 15 && double(dec - inc) / topwhitevalue >= 0.4) {
                            // 6%~14%, dec limit to 40% topvalue
                            oppwhite = semiwhite;
                        }
                    }
                }
                if (it.key() <= abswhite - (abswhite - absblack) / 3) {
                    break;
                }
            }
        } else {
            // slightly
            for (QMap < quint16, quint32 >::const_iterator it = grayhistogram.constBegin(); it != grayhistogram.constEnd(); it++) {
                oppblack = it.key();
                if (oppblack >= (fromgrayscale?128:32768)) {
                    break;
                }
                sum += it.value();
                if (sum > 0) {
                    if (pixelcount / sum < 100) {
                        break;
                    }
                }
            }
            sum = 0;
            for (QMap < quint16, quint32 >::const_iterator it = grayhistogram.constEnd(); it != grayhistogram.constBegin(); ) {
                it--;
                oppwhite = it.key();
                if (oppwhite <= (fromgrayscale?216:55296)) {
                    break;
                }
                sum += it.value();
                if (sum > 0) {
                    if (pixelcount / sum < 100) {
                        break;
                    }
                }
            }
        }
        qDebug("opposite black:%3d, opposite white:%3d", oppblack, oppwhite);
        if (oppblack == oppwhite) {
            // pure white / pure black?
            return;
        }
        // we can apply to each rgb channel?
        src = qimage.bits();
        if (is8bit) {
            for (int y = 0; y < qimage.height(); y++) {
                for (int x = 0; x < qimage.width(); x++) {
                    *src = qMin(255, qMax(0, ((*src - oppblack) * 255 / (oppwhite - oppblack)))); // RGB
                    src++;
                }
                src += qimage.bytesPerLine() - qimage.width();
            }
        } else {
            while (src < src_end)
            {
                if (fromgrayscale) {
                    *src = qMin(255, qMax(0, ((*src - oppblack) * 255 / (oppwhite - oppblack)))); // RGB
                    src[1] = src[0];
                    src[2] = src[0];
                    src += 3;
                } else {
                    for (int i = 0; i < 3; i++)
                    {
                        // ((10~245 * 256) - 2560)
                        *src = qMin(255, qMax(0, (((*src * 256) - oppblack) * 255 / (oppwhite - oppblack)))); // RGB
                        src++;
                    }
                }
                src++; // bypass alpha
            }
        }
    }
}

void PreparePosterizeImage(QImage& qimage)
{
    AutoExposureImage(qimage, false);
    QImage viaimage(qimage.width(), qimage.height(), QImage::Format_Indexed8);
    QVector < QRgb > grayscale2;
    for (QRgb i = 0; i <= 0xFFFFFF; i += 0x555555)
    {
        grayscale2.append(i | 0xFF000000);
    }
    viaimage.setColorTable(grayscale2);

    bool is8bit = qimage.depth() == 8;
    const uchar *src = qimage.bits();
    uchar* dest = viaimage.bits();
    if (is8bit) {
        for (int y = 0; y < qimage.height(); y++) {
            for (int x = 0; x < qimage.width(); x++) {
                *dest = qMin(3, (*src + 32) / 64); // 0~255 -> 0~3
                src++;
            }
            src += qimage.bytesPerLine() - qimage.width();
            dest += viaimage.bytesPerLine() - viaimage.width(); // same
        }
    } else {
        bool fromgrayscale = qimage.isGrayscale();
        for (int y = 0; y < qimage.height(); y++) {
            for (int x = 0; x < qimage.width(); x++) {
                if (fromgrayscale) {
                    *dest = qMin(3, (*src + 32) / 64);
                } else {
                    *dest = qMin(3, (src[0]*76 + src[1]*150 + src[2]*30) >> 12);
                }
                src+=4;
                dest++;
            }
            dest += viaimage.bytesPerLine() - viaimage.width(); // same
        }
    }
    qimage = viaimage; // always return 8bit indexed grayscale?
}


int CalculateGuideline( QImage &leftstripe, double sumlimits, double linelimits )
{
    const uchar *src = leftstripe.bits();
    int whiteline = 0, guidepos = 0;
    for (int y = 0; y < leftstripe.height(); y++) {
        int blacksum = 0;
        for (int x = 0; x < leftstripe.width(); x++) {
            blacksum += (3 - *src); // 0~3 -> 3~0
            src++;
        }
        src += leftstripe.bytesPerLine() - leftstripe.width();
        if (double(blacksum) / leftstripe.width() < sumlimits) {
            // 0.8%
            whiteline++;
            if (double(whiteline) / leftstripe.height() >= linelimits) {
                guidepos = leftstripe.height() - y + qMax(0, int((whiteline - 4) * 0.8)); // more care for pagenumber
                break;
            }
        } else {
            // it's a black line
            // -- or -= 2?
            whiteline = 0;
        }
    }
    return guidepos;
}

void AutocropImage( QImage& qimage, bool fromdualpage, bool isleftpage )
{
    // Safe~Dangerous
    // Left     3.1% 6.6%
    QTransform rotatetobottom;
    rotatetobottom.rotate(-90);
    QImage leftstripe = qimage.copy(0, 0, qimage.width() * 0.132, qimage.height()).transformed(rotatetobottom);
    // heavy level
    PreparePosterizeImage(leftstripe);
    // stripe became 4level now
    leftstripe = leftstripe.copy(0, leftstripe.height() - qimage.width() * 0.08, leftstripe.width(), qimage.width() * 0.08);
    int guidepos = CalculateGuideline(leftstripe, 0.015, ((fromdualpage&&isleftpage==false)?0.12:0.18));
    if (guidepos > 0) {
        qimage = qimage.copy(guidepos, 0, qimage.width() - guidepos, qimage.height());
    }

    // Right    4%   6.4%
    rotatetobottom.reset();
    rotatetobottom.rotate(90);
    QImage rightstripe = qimage.copy(qimage.width() - qimage.width() * 0.128, 0, qimage.width() * 0.128, qimage.height()).transformed(rotatetobottom);
    PreparePosterizeImage(rightstripe);
    rightstripe = rightstripe.copy(0, rightstripe.height() - qimage.width() * 0.08, rightstripe.width(), qimage.width() * 0.08);
    guidepos = CalculateGuideline(rightstripe, 0.015, ((fromdualpage&&isleftpage)?0.12:0.18));
    if (guidepos > 0) {
        qimage = qimage.copy(0, 0, qimage.width() - guidepos, qimage.height());
    }
    
    // Top      6.3% 9.2%
    QImage topstripe = qimage.copy(((fromdualpage&&isleftpage==false)?qimage.width() - qimage.width()*0.91:0), 0, (fromdualpage?qimage.width()*0.91:qimage.width()), qimage.height() * 0.184).mirrored(false, true); // TODO: rotate 180
    PreparePosterizeImage(topstripe);
    topstripe = topstripe.copy(0, topstripe.height() - qimage.height() * 0.11, topstripe.width(), qimage.height() * 0.11);
    guidepos = CalculateGuideline(topstripe, 0.012, 0.18);
    if (guidepos > 0) {
        qimage = qimage.copy(0, guidepos, qimage.width(), qimage.height() - guidepos);
    }

    // Bottom   5%   7.7%
    QImage bottomstripe = qimage.copy(((fromdualpage&&isleftpage==false)?qimage.width() - qimage.width()*0.91:0), qimage.height() - qimage.height() * 0.154, (fromdualpage?qimage.width()*0.91:qimage.width()), qimage.height() * 0.154);
    PreparePosterizeImage(bottomstripe);
    bottomstripe = bottomstripe.copy(0, bottomstripe.height() - qimage.height() * 0.09, bottomstripe.width(), qimage.height() * 0.09);
    guidepos = CalculateGuideline(bottomstripe, 0.006, 0.18); // pagenumber
    if (guidepos > 0) {
        qimage = qimage.copy(0, 0, qimage.width(), qimage.height() - guidepos);
    }

#ifdef _DEBUG
    leftstripe.save(QApplication::applicationDirPath() + "/leftstripe.png", "png", 80);
    rightstripe.save(QApplication::applicationDirPath() + "/rightstripe.png", "png", 80);
    topstripe.save(QApplication::applicationDirPath() + "/topstripe.png", "png", 80);
    bottomstripe.save(QApplication::applicationDirPath() + "/bottomstripe.png", "png", 80);
#endif

}

void ReplaceAlphaWithChecker( QImage& qimage )
{
    // TODO: optimize for huge piece
    // Inplace Draw, Inplace QImage format assign / Premulti
    QImage box(32, 32, qimage.format());
    QPainter pmp(&box);
    pmp.fillRect(0, 0, 16, 16, Qt::lightGray);
    pmp.fillRect(16, 16, 16, 16, Qt::lightGray);
    pmp.fillRect(0, 16, 16, 16, Qt::darkGray);
    pmp.fillRect(16, 0, 16, 16, Qt::darkGray);
    pmp.end();

    QImage viaimage = QImage(qimage);
    QPainter checkermaker(&qimage);
    QBrush checker;
    checker.setTextureImage(box);
    checkermaker.fillRect(qimage.rect(), checker);
    checkermaker.drawImage(0, 0, viaimage);
    checkermaker.end();

    if (qimage.format() == QImage::Format_ARGB32) {
        qimage = qimage.convertToFormat(QImage::Format_RGB32);
    }
}

void UncropImage( QImage& qimage, int destwidth, int destheight ) {
    if (qimage.depth() == 8) {
        Normalize8bitQImage(qimage);
    }
    const uchar *src = qimage.bits();
    int srcwidth = qimage.width();
    int srcheight = qimage.height();
    int heightdelta = destheight - srcheight;
    int widthdelta = destwidth - srcwidth;
    QImage viaimage = QImage(destwidth, destheight, qimage.format());
    uchar *dest = viaimage.bits();
    const uchar *srcend = src + srcheight * qimage.bytesPerLine();
    if (heightdelta > 1) {
        memset(dest, 0xFF, (heightdelta / 2) * viaimage.bytesPerLine());
        memset(dest + (destheight - (heightdelta - heightdelta / 2)) * viaimage.bytesPerLine(), 0xFF, (heightdelta - heightdelta / 2) * viaimage.bytesPerLine());
        dest += (heightdelta / 2) * viaimage.bytesPerLine();
    }
    if ((-heightdelta) > 1) {
        src += ((-heightdelta) / 2) * qimage.bytesPerLine();
        srcend -= ((-heightdelta) - (-heightdelta) / 2) * qimage.bytesPerLine();
    }
    int leftbypass = (widthdelta / 2) * (qimage.depth() / 8);
    int rightbypass = (widthdelta - widthdelta / 2) * (qimage.depth() / 8);
    int centerbyte = qMin(srcwidth, destwidth) * (qimage.depth() / 8);
    if (destwidth == srcwidth) {
        memcpy(dest, src, qMin(srcheight, destheight) * qimage.bytesPerLine());
    } else {
        while (src < srcend)
        {
            if (leftbypass > 0) {
                // Fill dest
                // 8 -> FF, 16-> FFFF, 32->FFFFFFFF
                memset(dest, 0xFF, leftbypass);
                dest += leftbypass;
            } else if (leftbypass < 0) {
                // bypass src. never run here
                src -= leftbypass;
            }
            memcpy(dest, src, centerbyte);
            src += centerbyte;
            dest += centerbyte;
            if (rightbypass > 0) {
                memset(dest, 0xFF, rightbypass);
                dest += rightbypass;
            } else if (rightbypass < 0) {
                src -= rightbypass;
            }
        }
    }
    qimage = viaimage;
}

bool IsGrayscaleImage( const QImage& qimage )
{
    bool Result = false;
    if (qimage.depth() == 8) {
        Result = true;
        if (qimage.colorTable().isEmpty() == false){
            foreach(QRgb color, qimage.colorTable()) {
                if (color % 0x010101) {
                    Result = false;
                    return Result;
                }
            }
        }
    } else {
        Result = qimage.isGrayscale(); // allGray
    }
    return Result;
}

int GetFreePhysMemory() {
    MEMORYSTATUS ms;
    ms.dwLength = sizeof (MEMORYSTATUS);
    GlobalMemoryStatus(&ms);
    return ms.dwAvailPhys;
}

//QImage NewQImageFromEXIFThumbnail( QFile& qfile )
//{
//    quint16 NextMarker = readuint16(qfile, true);
//    if (NextMarker != M_SOI) {
//        return QImage();
//    }
//    bool app1found = false, eoifound = false;
//    int app1offset;
//    while (app1found == false && eoifound == false) {
//        NextMarker = readuint16(qfile, true);
//        app1found = NextMarker == M_APP1;
//        eoifound = NextMarker == M_EOI;
//        if (app1found) {
//            app1offset = qfile.pos();
//            break;
//        }
//        if (NextMarker == M_SOS) { // SOS
//            // Jump more bytes
//            qfile.seek(qfile.size() - 2);
//            NextMarker = readuint16(qfile, true); // replace
//            // skip pending
//            int fpos2 = 0;
//            while(NextMarker != M_EOI) {
//                qfile.seek(qfile.size() - 2 - (++fpos2)); // rock back
//                NextMarker = readuint16(qfile, true);
//            } 
//        }
//        if (NextMarker != M_EOI) {
//            quint16 len = readuint16(qfile, true);
//            qfile.seek(qfile.pos() + len - sizeof(len));
//        }
//    }
//    if (app1found) {
//        quint16 len = readuint16(qfile, true);
//        QByteArray EXIF = qfile.read(6);
//        if (QString::fromAscii(EXIF.constData()) == "Exif") {
//            quint64 offset = qfile.pos(); // - EXIF.size();
//            QByteArray align = qfile.read(2);
//            bool islittleendian = align[0] == 'I';
//            quint16 tagMark = readuint16(qfile, !islittleendian); // 2a

//            quint32 offsetFirstIFD = readuint32(qfile, !islittleendian); // 8

//            if(offsetFirstIFD!=8) 
//                qfile.seek(offset + offsetFirstIFD);

//            // ifdMainImage
//            quint16 nDirEntry = readuint16(qfile, !islittleendian); // C

//            qfile.seek(qfile.pos() + 0xC * nDirEntry); // TODO: nComponent

//            quint32 nextIFDoffset = readuint32(qfile, !islittleendian); // 4E6

//            if (nextIFDoffset > 0) {
//                // ifdThumbnailImage
//                qfile.seek(offset + nextIFDoffset);
//                quint16 nDirEntry = readuint16(qfile, !islittleendian); // 8

//                //qfile.seek(qfile.pos() + 0xC * nDirEntry);
//                //quint32 nextIFDoffset;
//                qint32 thumbOffset = 0;
//                qint32 thumbLength = 0;
//                qint16 compression = 10;

//                for (int i = 0; i < nDirEntry; i++)
//                {
//                    quint16 tagNumber = readuint16(qfile, !islittleendian);

//                    switch (tagNumber)
//                    {
//                    case Compression:
//                        // + 8
//                        qfile.seek(qfile.pos() + 6); // format, nComponet
//                        compression = readuint16(qfile, !islittleendian);
//                        qfile.seek(qfile.pos() + 2); // padding
//                        break;
//                    case ThumbnailOffset:
//                    case StripOffsets:
//                        qfile.seek(qfile.pos() + 6);
//                        thumbOffset = readuint32(qfile, !islittleendian);
//                        break;
//                    case ThumbnailLength:
//                    case StripByteCounts:
//                        qfile.seek(qfile.pos() + 6);
//                        thumbLength = readuint32(qfile, !islittleendian);
//                        break;
//                    default:
//                        qfile.seek(qfile.pos() + 10);
//                    }
//                }

//                if(thumbLength && thumbOffset) {
//                    qfile.seek(offset + thumbOffset);
//                    //printf("Thumbnail Offset = %#Lx\n",offset + thumbOffset);
//                    if(compression == 6) {
//                        //local quad JpegFileEnd2 = JpegFileEnd;
//                        //JpegFileEnd = FTell() + thumbLength;
//                        //struct JPGFILE thumbnail;
//                        //JpegFileEnd = JpegFileEnd2;
//                        QByteArray thumbbuf = qfile.read(thumbLength);
//                        const QImage Result = QImage::fromData(thumbbuf);
//                        return Result;
//                    }
//                    else {
//                        //char imageData[thumbLength];
//                    }
//                }

//            }
//        }
//    }
//    return QImage();
//}

////////////////////////////////////////////////////////////////////////////////////////////////////
// sticky safe conversation from ANSI string to wide string
wstring StringToWideString(const string& Source, unsigned int Codepage) {
    int len;
#ifdef _WIN32
    len = MultiByteToWideChar(Codepage, MB_PRECOMPOSED, Source.c_str(), static_cast<int>(Source.length()), NULL, 0);
#elif defined _LINUX
    char* oldlocale	= setlocale(LC_CTYPE, "GBK");
    char* savedlocale = strdup(oldlocale);
    len = mbstowcs(NULL, Source.c_str(), 0);
#endif
    if (0 >= len){
        return wstring(L"");
    } else {
        WCHAR * DestBuf = LPWSTR(malloc(sizeof(WCHAR)*(len + 1)));
        if (NULL==DestBuf) {
            return wstring(L"");
        } 
        DestBuf[len] = '\0';
#ifdef _WIN32
        bool done = (0 <= MultiByteToWideChar(Codepage, MB_PRECOMPOSED, Source.c_str(), static_cast<int>(Source.length()), DestBuf, len));
#elif defined _LINUX
        bool done = (0 <= mbstowcs(DestBuf, Source.c_str(), Source.length()));
        (void)setlocale(LC_CTYPE, savedlocale);
        free(savedlocale);
#endif
        if (done) {
            const wstring Result = wstring(DestBuf);
            free(DestBuf);
            return Result;
        } else {
            free(DestBuf);
            return wstring(L"");
        }
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////

quint16 readuint16( QIODevice& qfile, bool bigendian /*= false*/, bool peek /*= false*/ )
{
    quint16 Result;
    qfile.read((char*)&Result, sizeof(Result));
    if (bigendian) {
        Result = qFromBigEndian(Result);
    } else {
        Result = qFromLittleEndian(Result);
    }
    if (peek) {
        qfile.seek(qfile.pos() - sizeof(Result));
    }
    return Result;
}

quint32 readuint32( QIODevice& qfile, bool bigendian /*= false*/, bool peek /*= false*/ )
{
    quint32 Result;
    qfile.read((char*)&Result, sizeof(Result));
    if (bigendian) {
        Result = qFromBigEndian(Result);
    } else {
        Result = qFromLittleEndian(Result);
    }
    if (peek) {
        qfile.seek(qfile.pos() - sizeof(Result));
    }
    return Result;
}

TImageType GuessMIMEType(QIODevice* file) {
    TImageType Result = itUnknown;
    if (file == NULL) {
        return Result;
    }
    if (!file->isOpen()) {
        file->open(QIODevice::ReadOnly);
    }
    if (file->isReadable() == false) {
        return Result;
    }
    quint8 headerbuf[6] = {0};
    file->read(reinterpret_cast<char*>(&headerbuf[0]), sizeof headerbuf);

    if (* reinterpret_cast< quint32* >(&headerbuf[0]) == 0x00424949 || * reinterpret_cast< quint32* >(&headerbuf[0]) == 0x42004D4D) {
        // II4200, MM0042
        Result = itTarga;
    }
    if (* reinterpret_cast< quint16* >(&headerbuf[0]) == 0x4D42) {
        // BM
        Result = itBitmap;
    }
    if (* reinterpret_cast< quint16* >(&headerbuf[0]) == 0xD8FF) {
        // JFIF Magic
        Result = itJPEG;
    }
    if (* reinterpret_cast< quint32* >(&headerbuf[0]) == 0x38464947 && headerbuf[4] == 0x39) {
        // GIF89 Magic
        Result = itGif;
    }
    if (* reinterpret_cast< quint32* >(&headerbuf[0]) == 0x474E5089) {
        // JFIF Magic
        Result = itPNG;
    }
    if (* reinterpret_cast< quint32* >(&headerbuf[0]) == 0x01BC4949 || * reinterpret_cast< quint32* >(&headerbuf[0]) == 0xBC014D4D) {
        // IIBC01, MM01BC
        Result = itHDPhoto;
    }
    if (* reinterpret_cast< quint32* >(&headerbuf[0]) == 0x04034B50) {
        // PK Magic
        Result = itPKArchive;
    }
    if (* reinterpret_cast< quint32* >(&headerbuf[0]) == 0x21726152 && headerbuf[4] == 0x1A && headerbuf[5] == 0x07) {
        // Rar!1A07 Magic
        Result = itRarArchive;
    }
    return Result;
}

TImageType GuessMIMEType(const QString& filename) {
    TImageType Result = itUnknown;

    QFile inputfile(filename);

    Result = GuessMIMEType(&inputfile);

    inputfile.close();
    return Result;
}



extern "C" {
int /*Q_ZEXPORT*/ uncompress2 (
Bytef *dest,
uLongf *destLen,
const Bytef *source,
uLong sourceLen)
{
    z_stream stream;
    int err;

    stream.next_in = (Bytef*)source;
    stream.avail_in = (uInt)sourceLen;
    stream.total_in = (uInt)sourceLen;
    /* Check for source > 64K on 16-bit machine: */
    if ((uLong)stream.avail_in != sourceLen) return Z_BUF_ERROR;

    stream.next_out = dest;
    stream.avail_out = (uInt)*destLen;
    stream.total_out = (uInt)*destLen;
    if ((uLong)stream.avail_out != *destLen) return Z_BUF_ERROR;

    stream.zalloc = (alloc_func)0;
    stream.zfree = (free_func)0;

    err = inflateInit2(&stream, -MAX_WBITS);

    if (err != Z_OK) return err;

    err = inflate(&stream, Z_FINISH);

    if (err != Z_STREAM_END) {
        inflateEnd(&stream);
        if (err == Z_NEED_DICT || (err == Z_BUF_ERROR && stream.avail_in == 0))
            return Z_DATA_ERROR;
        return err;
    }
    *destLen = stream.total_out;

    err = inflateEnd(&stream);
    return err;
}
}

// take from qbytearray.cpp
QByteArray qUncompressEx(const uchar* data, int nbytes, int outputbytes)
{
    if (!data) {
        qWarning("qUncompressEx: Data is null");
        return QByteArray();
    }
    if (nbytes <= 0) {
        if (nbytes < 0 || (outputbytes != 0))
            qWarning("qUncompressEx: Input data is corrupted");
        return QByteArray();
    }
    ulong expectedSize = outputbytes;
    ulong len = qMax(expectedSize, 1ul);
    QByteArray baunzip;
    int res;
    do {
        baunzip.resize(len);
        res = ::uncompress2((uchar*)baunzip.data(), &len,
            (uchar*)data, nbytes);

        switch (res) {
        case Z_OK:
            if ((int)len != baunzip.size())
                baunzip.resize(len);
            break;
        case Z_MEM_ERROR:
            qWarning("qUncompressEx: Z_MEM_ERROR: Not enough memory");
            break;
        case Z_BUF_ERROR:
            len *= 2;
            break;
        case Z_DATA_ERROR:
            qWarning("qUncompressEx: Z_DATA_ERROR: Input data is corrupted");
            break;
        }
    } while (res == Z_BUF_ERROR);

    if (res != Z_OK)
        baunzip = QByteArray();

    return baunzip; // @CopyRecord?
}

QByteArray qUncompressEx1(const uchar* data, int nbytes, int outputbytes)
{
    if (!data) {
        qWarning("qUncompressEx: Data is null");
        return QByteArray();
    }
    if (nbytes <= 0) {
        if (nbytes < 0 || (outputbytes != 0))
            qWarning("qUncompressEx: Input data is corrupted");
        return QByteArray();
    }
    ulong expectedSize = outputbytes;
    ulong len = qMax(expectedSize, 1ul);
    QByteArray baunzip;
    int res;
    do {
        baunzip.resize(len);
        res = ::uncompress((uchar*)baunzip.data(), &len,
            (uchar*)data, nbytes);

        switch (res) {
        case Z_OK:
            if ((int)len != baunzip.size())
                baunzip.resize(len);
            break;
        case Z_MEM_ERROR:
            qWarning("qUncompressEx: Z_MEM_ERROR: Not enough memory");
            break;
        case Z_BUF_ERROR:
            len *= 2;
            break;
        case Z_DATA_ERROR:
            qWarning("qUncompressEx: Z_DATA_ERROR: Input data is corrupted");
            break;
        }
    } while (res == Z_BUF_ERROR);

    if (res != Z_OK)
        baunzip = QByteArray();

    return baunzip; // @CopyRecord?
}

QByteArray qCompressEx(const uchar* data, int nbytes, int compressionLevel)
{
    if (nbytes == 0) {
        return QByteArray(4, '\0');
    }
    if (!data) {
        qWarning("qCompressEx: Data is null");
        return QByteArray();
    }
    if (compressionLevel < -1 || compressionLevel > 9)
        compressionLevel = -1;

    ulong len = nbytes + nbytes / 100 + 13;
    QByteArray bazip;
    int res;
    do {
        bazip.resize(len);
        res = ::compress2((uchar*)bazip.data(), &len, (uchar*)data, nbytes, compressionLevel);

        switch (res) {
        case Z_OK:
            bazip.resize(len);
            break;
        case Z_MEM_ERROR:
            qWarning("qCompressEx: Z_MEM_ERROR: Not enough memory");
            bazip.resize(0);
            break;
        case Z_BUF_ERROR:
            len *= 2;
            break;
        }
    } while (res == Z_BUF_ERROR);

    return bazip;
}

TImageType ExtToMIMEType( const QString& extname )
{
    TImageType Result = itUnknown;
    QString pureext = extname.toLower(); // TODO: remove dot
    if (pureext == "tga") {
        Result = itTarga;
    }
    if (pureext == "bmp") {
        Result = itBitmap;
    }
    if (pureext == "png") {
        Result = itPNG;
    }
    if (pureext == "jpg") {
        Result = itJPEG;
    }
    if (pureext == "gif") {
        Result = itGif;
    }
    if (pureext == "wdp") {
        Result = itHDPhoto;
    }
    if (pureext == "zip") {
        Result = itPKArchive;
    }
    if (pureext == "rar") {
        Result = itRarArchive;
    }
    return Result;
}

QString MIMETypeToExt( TImageType type )
{
    QString Result = "";
    switch (type)
    {
    case itUnknown:
        Result = "unknown";
        break;
    case itBitmap:
        Result = "bmp";
        break;
    case itPNG:
        Result = "png";
        break;
    case itJPEG:
        Result = "jpg";
        break;
    case itGif:
        Result = "gif";
        break;
    case itHDPhoto:
        Result = "wdp";
        break;
    case itTarga:
        Result = "tga";
        break;
    case itPKArchive:
        Result = "zip";
        break;
    case itRarArchive:
        Result = "rar";
        break;
    }
    return Result;
}

int greedydiv( int n, int m )
{
    int Result = (n + m - 1) / m;
    return Result;
}

bool SaveByteArrayToFile( const QString& filename, const QByteArray& content )
{
    QDir dir(QFileInfo(filename).path());
    if (dir.exists() == false) {
        QDir::root().mkpath(dir.path());
        //_wmkdir(QDir::toNativeSeparators(QFileInfo(filename).path()).utf16());
    }
    QFile file(filename);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(content);
        file.close();
        return true;
    }
    return false;
}

/*****************************************************************************
 *
 * static functions used to init Class PDefaults
 *
 *****************************************************************************/

int getCpuCount()
{
    int cpuCount = 1;

#if defined(Q_OS_WIN32) || defined(Q_OS_WIN64)
    {
	SYSTEM_INFO    si;
	GetSystemInfo(&si);
	cpuCount = si.dwNumberOfProcessors;
    }
#elif defined(Q_OS_UNIX) && !defined(Q_OS_MACX)
    cpuCount = sysconf(_SC_NPROCESSORS_ONLN);
#elif defined(Q_OS_MACX)
   kern_return_t		kr;
   struct host_basic_info	hostinfo;
   unsigned int			count;

   count = HOST_BASIC_INFO_COUNT;
   kr = host_info(mach_host_self(), HOST_BASIC_INFO, (host_info_t)&hostinfo, &count);
   if(kr == KERN_SUCCESS) {
	cpuCount = hostinfo.avail_cpus;
//	totalMemory = hostinfo.memory_size; 			 //in bytes
   }

#endif

    if( cpuCount < 1 )
	cpuCount = 1;

    return cpuCount;
}

unsigned int getTickCount() {
    return GetTickCount();
}