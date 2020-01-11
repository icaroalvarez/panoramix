#pragma once
#include "ImageProcessor.h"
#include "Factory.h"
#include <any>
#include <opencv2/core/mat.hpp>

class PipelineController
{
public:

    void loadPipeline(const std::vector<std::string>& imageProcessorNames);

    void loadImage(const std::string &path);

    void configureProcessor(unsigned int index, const Configuration& configuration);

    template <typename T>
    void registerImageProcessor(const std::string& processorName)
    {
        imageProcessorFactory.registerMaker<T>(processorName);
    }

private:
    std::vector<std::unique_ptr<ImageProcessor>> imageProcessors;
    Factory<ImageProcessor> imageProcessorFactory;
    cv::Mat currentImage;
};