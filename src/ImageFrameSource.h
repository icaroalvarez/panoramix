#pragma once
#include "FrameSource.h"

class ImageFrameSource:public FrameSource
{
public:
    void loadFrom(const std::string& path) override;

    unsigned int framesCount() const override;

    cv::Mat getFrameFromIndex(unsigned int index) override;

private:
    cv::Mat image;
};



