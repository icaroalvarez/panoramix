#include <opencv2/imgcodecs.hpp>
#include <thread>
#include "PipelineController.h"
#include "FrameSourceFactory.h"
#include "easylogging++.h"
#include "JsonStorage.h"

PipelineController::PipelineController()
:PipelineController(std::make_unique<FrameSourceFactory>())
{
}

PipelineController::PipelineController(std::unique_ptr<FrameSourceFactory> frameSourceFactory)
:frameSourceFactory{std::move(frameSourceFactory)}
{
}

void PipelineController::loadPipeline(const PipelineConfiguration &configuration)
{
    loadFrameSourceFrom(configuration.frameSourcePath);

    std::string notRegisteredNames;
    for(const auto& processorTuple : configuration.imageProcessors)
    {
        const auto& [processorName, processorConfiguration] = processorTuple;

        try
        {
            imageProcessors.emplace_back(imageProcessorFactory.make(processorName));
            imageProcessors.back()->getParameters().configure(processorConfiguration);
        }catch(const std::invalid_argument&)
        {
            notRegisteredNames.append(processorName+", ");
        }
    }

    if(notRegisteredNames.size() > 2)
    {
        notRegisteredNames.pop_back();
        notRegisteredNames.pop_back();
        imageProcessors.clear();
        throw (std::invalid_argument("Couldn't load pipeline, processors not found: " + notRegisteredNames));
    }
}

void PipelineController::loadPipelineFromJson(const nlohmann::json &configurationFile)
{
    const auto& pipelineConfiguration{JsonStorage::createPipelineConfigurationFromJson(configurationFile)};
    loadPipeline(pipelineConfiguration);
}

void PipelineController::loadPipelineFromJsonFile(const std::string& path)
{
    const auto config = JsonStorage::createPipelineConfigurationFromJsonFile(path);
    const auto& pipelineConfiguration{JsonStorage::createPipelineConfigurationFromJsonFile(path)};
    loadPipeline(pipelineConfiguration);
}

void PipelineController::loadFrameSourceFrom(const std::string &path)
{
    frameSource = std::move(frameSourceFactory->createAndLoadFromPath(path));
    frameSourcePath = path;
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
    imageProcessors[index]->getParameters().configure(configuration);
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

cv::Mat PipelineController::getDebugImageFrom(unsigned int processorIndex)
{
    checkImageProcessorRange(imageProcessors, processorIndex, "Couldn't debug image. ");
    return imageProcessors[processorIndex]->getDebugImage();
}

Parameters PipelineController::getProcessorParameters(unsigned int processorIndex) const {
    checkImageProcessorRange(imageProcessors, processorIndex, "Couldn't get image processor parameters. ");
    return imageProcessors[processorIndex]->getParameters().getParameters();
}

void PipelineController::runIteration()
{
    try
    {
        currentImage = frameSource->getFrameFromIndex(frameSourceIndex);
        cv::Mat imageFromPreviousProcessor{currentImage};
        for(const auto& processor: imageProcessors)
        {
            imageFromPreviousProcessor = processor->processImage(imageFromPreviousProcessor);
        }
        notifyObservers();
    }
    catch(const std::exception& e)
    {
        LOG(ERROR) << "Error running pipeline controller: " << e.what();
    }
}

void PipelineController::setFrameSourceIndex(unsigned index)
{
    if(not frameSource)
    {
        throw std::runtime_error("Frame source is not loaded yet, index cannot be set");
    }
    const auto framesCount{frameSource->framesCount()};
    if(index >= framesCount)
    {
        const auto maxIndex{std::max(static_cast<int>(framesCount-1), 0)};
        throw std::invalid_argument("Frame source index out of bound (requested: "+
                                    std::to_string(index)+", max: "+std::to_string(maxIndex)+")");
    }
    frameSourceIndex = index;
}

unsigned int PipelineController::getTotalFrames() const
{
    return frameSource->framesCount();
}

// overloaded helper from std::variant documentation
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>; // not needed as of C++20
PipelineConfiguration PipelineController::getPipelineConfiguration() const
{
    PipelineConfiguration pipelineConfiguration;
    pipelineConfiguration.frameSourcePath = frameSourcePath;
    for(const auto& processor : imageProcessors)
    {
        const auto& parameters{processor->getParameters().getParameters()};
        Configuration processorConfiguration;
        for(const auto& parameter : parameters)
        {
            const auto& parameterName{parameter.first};
            std::visit(overloaded{
                    [&](IntegerParameter integerParameter)
                    {
                        processorConfiguration[parameterName] = integerParameter.value;
                    },
                    [&](DecimalParameter decimalParameter)
                    {
                        processorConfiguration[parameterName] = decimalParameter.value;
                    },
                    [&](bool value)
                    {
                        processorConfiguration[parameterName] = value;
                    },
                    [&](OptionsParameter optionsParameter)
                    {
                        processorConfiguration[parameterName] = optionsParameter.selectedOptionIndex;
                    }
            }, parameter.second);
        }
        pipelineConfiguration.imageProcessors.emplace_back(processor->getName(), std::move(processorConfiguration));
    }
    return pipelineConfiguration;
}

void PipelineController::savePipelineConfigurationTo(const std::string &path) const
{
    const auto configuration{getPipelineConfiguration()};
    JsonStorage::saveProcessorConfigurationTo(configuration, path);
}
