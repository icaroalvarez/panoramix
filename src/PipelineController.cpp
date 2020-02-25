#include <opencv2/imgcodecs.hpp>
#include <thread>
#include "PipelineController.h"

void PipelineController::loadPipeline(const std::vector<std::string>& imageProcessorNames)
{
    std::string notRegisteredNames;
    for(const auto& name : imageProcessorNames)
    {
        try
        {
            imageProcessors.emplace_back(imageProcessorFactory.make(name));
        }catch(const std::invalid_argument&)
        {
            notRegisteredNames.append(name+", ");
        }
    }
    if(notRegisteredNames.size() > 2)
    {
        notRegisteredNames.pop_back();
        notRegisteredNames.pop_back();
        throw(std::invalid_argument("Couldn't load pipeline, processors not found: "+notRegisteredNames));
    }
}

std::vector<std::string> PipelineController::getPipelineDescription() const
{
    std::vector<std::string> processorNames;
    for (const auto &processor : imageProcessors)
    {
        processorNames.push_back(processor->getName());
    }
    return processorNames;
}

void PipelineController::loadFrameSourceFrom(const std::string &path)
{
    frameSource = std::make_unique<ImageFrameSource>();
    frameSource->loadFrom(path);
    frameSourceIndex = 0;
}

void checkImageProcessorRange(const std::vector<std::unique_ptr<ImageProcessor>>& imageProcessors,
                              unsigned int index,
                              const std::string& exceptionMessage)
{
    if(index >= imageProcessors.size())
    {
        throw std::invalid_argument(exceptionMessage+"The index "
                                    +std::to_string(index)+" is out of range ("
                                    +std::to_string(imageProcessors.size())+")");
    }
}

void PipelineController::configureProcessor(unsigned int index,
                                            const Configuration& configuration)
{
    checkImageProcessorRange(imageProcessors, index, "Couldn't configure the image processor. ");
    imageProcessors[index]->getConfiguration().configure(configuration);
}

void PipelineController::firePipelineProcessing()
{
    fireIteration();
}

void PipelineController::addImageProcessor(std::unique_ptr<ImageProcessor> imageProcessor)
{
    imageProcessors.emplace_back(std::move(imageProcessor));
}

cv::Mat PipelineController::getCurrentLoadedImage()
{
    return currentImage;
}

cv::Mat PipelineController::getDebugImage(unsigned int processorIndex)
{
    checkImageProcessorRange(imageProcessors, processorIndex, "Couldn't debug image. ");
    return imageProcessors[processorIndex]->getDebugImage();
}

Parameters PipelineController::getProcessorConfigurationFrom(unsigned int processorIndex) const {
    checkImageProcessorRange(imageProcessors, processorIndex, "Couldn't get image processor configuration. ");
    return imageProcessors[processorIndex]->getConfiguration().getParameters();
}

void PipelineController::runIteration()
{
    currentImage = frameSource->getFrameFromIndex(frameSourceIndex);
    cv::Mat imageFromPreviousProcessor{currentImage};
    for(const auto& processor: imageProcessors)
    {
        processor->processImage(imageFromPreviousProcessor);
    }
    notifyObservers();
}

void PipelineController::setFrameSourceIndex(unsigned index)
{
    const auto framesCount{frameSource->framesCount()};
    if(index >= framesCount)
    {
        throw std::invalid_argument("Frame source index out of bound (requested: "+
                                    std::to_string(index)+" max: "+std::to_string(framesCount));
    }
    frameSourceIndex = index;
}
