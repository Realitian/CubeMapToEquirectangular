//
//  main.cpp
//  cubemapToEquirectangular
//
//  Created by apple on 2019/10/20.
//  Copyright © 2019 VisionLab. All rights reserved.
//

#include <ibl/Cubemap.h>
#include <ibl/CubemapIBL.h>
#include <ibl/CubemapSH.h>
#include <ibl/CubemapUtils.h>
#include <ibl/Image.h>
#include <ibl/utilities.h>

#include <imageio/BlockCompression.h>
#include <imageio/ImageDecoder.h>
#include <imageio/ImageEncoder.h>

#include <image/KtxBundle.h>
#include <image/ColorTransform.h>

#include <utils/JobSystem.h>
#include <utils/Path.h>
#include <utils/algorithm.h>

#include <math/scalar.h>
#include <math/vec4.h>

#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace filament::math;
using namespace filament::ibl;
using namespace image;

static LinearImage toLinearImage(const Image& image);

int main(int argc, const char * argv[]) {
    utils::JobSystem js;
    js.adopt();
    
    std::string iname[] = {
        "/Users/apple/Desktop/FootprintCourt/posx.exr",
        "/Users/apple/Desktop/FootprintCourt/negx.exr",
        "/Users/apple/Desktop/FootprintCourt/posy.exr",
        "/Users/apple/Desktop/FootprintCourt/negy.exr",
        "/Users/apple/Desktop/FootprintCourt/posz.exr",
        "/Users/apple/Desktop/FootprintCourt/negz.exr",
    };
    
    Cubemap::Face faces[] = {
        Cubemap::Face::PX,
        Cubemap::Face::NX,
        Cubemap::Face::PY,
        Cubemap::Face::NY,
        Cubemap::Face::PZ,
        Cubemap::Face::NZ,
    };

    Image temp;
    Cubemap cml = CubemapUtils::create(temp, 512);
    
    Image* inputImages[6];

    for ( int i = 0 ; i < 6 ; i++) {
        std::ifstream input_stream(iname[i], std::ios::binary);
        LinearImage linputImage = ImageDecoder::decode(input_stream, iname[i]);
        if (!linputImage.isValid()) {
            std::cerr << "Unable to open image: " << iname[i] << std::endl;
            exit(1);
        }
        if (linputImage.getChannels() != 3) {
            std::cerr << "Input image must be RGB (3 channels)! This image has "
                      << linputImage.getChannels() << " channels." << std::endl;
            exit(1);
        }

        // Convert from LinearImage to the deprecated Image object which is used throughout cmgen.
        const size_t width = linputImage.getWidth(), height = linputImage.getHeight();
        inputImages[i] = new Image(width, height);
        memcpy(inputImages[i]->getData(), linputImage.getPixelRef(), height * inputImages[i]->getBytesPerRow());

        cml.setImageForFace(faces[i], *inputImages[i]);
    };
    
    Image outImage(1024 * 2, 1024);
    CubemapUtils::cubemapToEquirectangular(js, outImage, cml);
    
    std::string filename = "/Users/apple/Desktop/FootprintCourt/equirectangular.hdr";
    std::ofstream outputStream(filename, std::ios::binary | std::ios::trunc);
    if (!ImageEncoder::encode(outputStream, ImageEncoder::Format::HDR, toLinearImage(outImage), "", filename)) {
        exit(1);
    }
    
    return 0;
}

static LinearImage toLinearImage(const Image& image) {
    LinearImage linearImage((uint32_t) image.getWidth(), (uint32_t) image.getHeight(), 3);

    // Copy row by row since the image has padding.
    assert(image.getBytesPerPixel() == 12);
    const size_t w = image.getWidth(), h = image.getHeight();
    for (size_t row = 0; row < h; ++row) {
        float* dst = linearImage.getPixelRef(0, (uint32_t) row);
        float const* src = static_cast<float const*>(image.getPixelRef(0, row));
        memcpy(dst, src, w * 12);
    }
    return linearImage;
}
