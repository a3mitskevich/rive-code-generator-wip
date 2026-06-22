#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "CLIUTILS/CLI11.hpp"
#include "console_output.h"
#include "default_template.h"
#include "kainjow/mustache.hpp"
#include "rive/animation/linear_animation_instance.hpp"
#include "rive/animation/state_machine_input_instance.hpp"
#include "rive/animation/state_machine_instance.hpp"
#include "rive/assets/audio_asset.hpp"
#include "rive/assets/font_asset.hpp"
#include "rive/assets/image_asset.hpp"
#include "rive/file.hpp"
#include "rive/generated/animation/state_machine_bool_base.hpp"
#include "rive/generated/animation/state_machine_number_base.hpp"
#include "rive/generated/animation/state_machine_trigger_base.hpp"
#include "rive/viewmodel/data_enum.hpp"
#include "rive/viewmodel/data_enum_value.hpp"
#include "rive/viewmodel/runtime/viewmodel_runtime.hpp"
#include "rive/viewmodel/viewmodel_property_enum.hpp"
#include "rive/animation/linear_animation.hpp"
#include "rive/animation/loop.hpp"
#include "rive/event.hpp"
#include "rive/open_url_event.hpp"
#include "rive/audio_event.hpp"
#include "rive/custom_property_number.hpp"
#include "rive/custom_property_boolean.hpp"
#include "rive/custom_property_string.hpp"
#include "rive/custom_property_color.hpp"
#include "rive/custom_property_enum.hpp"
#include "rive/custom_property_trigger.hpp"
#include "rive/animation/state_machine_layer.hpp"
#include "rive/animation/layer_state.hpp"
#include "rive/animation/animation_state.hpp"
#include "rive/animation/any_state.hpp"
#include "rive/animation/entry_state.hpp"
#include "rive/animation/exit_state.hpp"
#include "rive/animation/state_transition.hpp"
#include "rive/text/text.hpp"
#include "rive/text/text_value_run.hpp"
#include "rive/text/text_style.hpp"
#include "rive/text/text_style_paint.hpp"
#include "rive/text_engine.hpp"
#include "rive/viewmodel/viewmodel.hpp"
#include "rive/viewmodel/viewmodel_instance.hpp"
#include "rive/viewmodel/viewmodel_instance_string.hpp"
#include "rive/viewmodel/viewmodel_instance_number.hpp"
#include "rive/viewmodel/viewmodel_instance_boolean.hpp"
#include "rive/viewmodel/viewmodel_instance_color.hpp"
#include "rive/viewmodel/viewmodel_instance_enum.hpp"
#include "rive/viewmodel/viewmodel_instance_value.hpp"
#include "rive/viewmodel/viewmodel_property_viewmodel.hpp"
#include "rive/viewmodel/runtime/viewmodel_instance_runtime.hpp"
#include "rive/viewmodel/runtime/viewmodel_instance_list_runtime.hpp"
#include "utils/no_op_factory.hpp"

const std::string generatedFileName = "rive_generated";

enum class CaseStyle
{
    Camel,
    Pascal,
    Snake,
    Kebab,
};

enum class Language
{
    Dart,
    JavaScript
};

struct InputInfo
{
    std::string name;
    std::string type;
    std::string defaultValue;
};

struct TextValueRunInfo
{
    std::string name;
    std::string defaultValue;
    float fontSize = 0.f;
    float lineHeight = 0.f;
    float letterSpacing = 0.f;
    std::string fontAssetId;
    std::string align;         // left | right | center
    std::string verticalAlign; // top | middle | bottom
    std::string sizing;        // autoWidth | autoHeight | fixed
    std::string overflow;      // visible | hidden | clipped | ellipsis | fit
    std::string wrap;          // wrap | noWrap
};

struct NestedTextValueRunInfo
{
    std::string name;
    std::string path;
};

struct AssetInfo
{
    std::string name;
    std::string type;
    std::string fileExtension;
    std::string assetId;
    std::string cdnUuid;
    std::string cdnBaseUrl;
    std::string uniqueFilename;
    bool isEmbedded = false;
    float width = 0.f;
    float height = 0.f;
};

struct AnimationInfo
{
    std::string name;
    uint32_t fps = 0;
    float durationSeconds = 0.f;
    std::string loop; // oneShot | loop | pingPong
    float speed = 1.f;
};

struct EventPropertyInfo
{
    std::string name;
    std::string type; // number | boolean | string | color | enum | trigger
    std::string defaultValue;
};

struct EventInfo
{
    std::string name;
    std::string type; // general | openUrl | audio
    std::string url;
    std::string target;
    std::string assetId;
    std::vector<EventPropertyInfo> properties;
};

struct EnumValueInfo
{
    std::string key;
    std::string value;
};

struct EnumInfo
{
    std::string name;
    std::vector<EnumValueInfo> values;
};

struct PropertyInfo
{
    std::string name;
    std::string type;
    std::string backingName;
    std::string defaultValue;
    bool hasDefaultValue = false;
};

struct ViewModelInfo
{
    std::string name;
    std::vector<PropertyInfo> properties;
    std::vector<std::string> instanceNames;
};

struct StateInfo
{
    std::string name;
    std::string type; // entry | exit | any | animation | other
    std::vector<std::string> transitions; // target state names
};

struct StateMachineInfo
{
    std::string name;
    std::vector<InputInfo> inputs;
    std::vector<StateInfo> states;
};

struct ArtboardData
{
    std::string artboardName;
    std::string artboardPascalCase;
    std::string artboardCameCase;
    std::string artboardSnakeCase;
    std::string artboardKebabCase;
    std::vector<AnimationInfo> animations;
    std::vector<StateMachineInfo> stateMachines;
    std::vector<TextValueRunInfo> textValueRuns;
    std::vector<NestedTextValueRunInfo> nestedTextValueRuns;
    std::vector<EventInfo> events;
    float width = 0.f;
    float height = 0.f;
    float originX = 0.f;
    float originY = 0.f;
    bool clip = false;
    bool hasDefaultStateMachine = false;
    std::string defaultStateMachineName;
    bool hasBoundViewModel = false;
    std::string boundViewModelName;
};

struct RiveFileData
{
    std::string rivName;
    std::string rivPascalCase;
    std::string rivCameCase;
    std::string riveSnakeCase;
    std::string rivKebabCase;
    std::vector<ArtboardData> artboards;
    std::vector<AssetInfo> assets;
    std::vector<EnumInfo> enums;
    std::vector<ViewModelInfo> viewmodels;
};

// Helper function to convert a string to the specified case style
static std::string toCaseHelper(const std::string& str, CaseStyle style)
{
    std::stringstream result;
    bool capitalizeNext = (style == CaseStyle::Pascal);
    bool firstChar = true;

    // Check if the first character is a digit
    if (std::isdigit(str[0]))
    {
        result << 'n';         // Prepend 'n' for number
        capitalizeNext = true; // Capitalize the first digit
        firstChar = false;
    }

    // Process the string
    for (size_t i = 0; i < str.length(); i++)
    {
        char c = str[i];

        if (std::isalnum(c))
        {
            if (capitalizeNext)
            {
                result << (char)std::toupper(c);
                capitalizeNext = false;
            }
            else
            {
                result << (style == CaseStyle::Pascal ? c
                                                      : (char)std::tolower(c));
            }
            firstChar = false;
        }
        else if (c == ' ' || c == '_' || c == '-')
        {
            if (!firstChar)
            {
                switch (style)
                {
                    case CaseStyle::Camel:
                    case CaseStyle::Pascal:
                        capitalizeNext = true;
                        break;
                    case CaseStyle::Snake:
                        result << '_';
                        break;
                    case CaseStyle::Kebab:
                        result << '-';
                        break;
                }
            }
        }
        // All other characters are ignored
    }

    // Ensure the result is not empty and starts with a letter
    std::string finalResult = result.str();
    if (finalResult.empty() || !std::isalpha(finalResult[0]))
    {
        finalResult = "X" + finalResult;
    }

    return finalResult;
}

static std::string toCamelCase(const std::string& str)
{
    std::string result = toCaseHelper(str, CaseStyle::Camel);
    // TODO: These handlers are generic to dart, we need to make something more
    // generic to handle all languages
    // Handle Dart reserved keywords
    if (result == "with" || result == "class" || result == "enum" ||
        result == "var" || result == "const" || result == "final" ||
        result == "static" || result == "void" || result == "int" ||
        result == "double" || result == "bool" || result == "String" ||
        result == "List" || result == "Map" || result == "dynamic" ||
        result == "null" || result == "true" || result == "false")
    {
        result = result + "Value";
    }
    return result;
}

static std::string toPascalCase(const std::string& str)
{
    return toCaseHelper(str, CaseStyle::Pascal);
}

static std::string toSnakeCase(const std::string& str)
{
    return toCaseHelper(str, CaseStyle::Snake);
}

static std::string toKebabCase(const std::string& str)
{
    return toCaseHelper(str, CaseStyle::Kebab);
}

static std::string sanitizeString(const std::string& input)
{
    std::string output;
    for (char c : input)
    {
        switch (c)
        {
            case '\n':
                output += "\\n";
                break;
            case '\r':
                output += "\\r";
                break;
            case '\t':
                output += "\\t";
                break;
            case '\"':
                output += "\\\"";
                break;
            case '\\':
                output += "\\\\";
                break;
            default:
                if (std::isprint(c))
                {
                    output += c;
                }
                else
                {
                    char hex[7];
                    std::snprintf(hex,
                                  sizeof(hex),
                                  "\\u%04x",
                                  static_cast<unsigned char>(c));
                    output += hex;
                }
        }
    }
    return output;
}

// Format a float without trailing zeros (e.g. 1920.0 -> "1920", 0.5 -> "0.5")
static std::string formatNumber(float value)
{
    std::ostringstream out;
    out << value;
    return out.str();
}

static std::string loopToString(rive::Loop loop)
{
    switch (loop)
    {
        case rive::Loop::oneShot:
            return "oneShot";
        case rive::Loop::loop:
            return "loop";
        case rive::Loop::pingPong:
            return "pingPong";
        default:
            return "oneShot";
    }
}

static std::string textAlignToString(rive::TextAlign align)
{
    switch (align)
    {
        case rive::TextAlign::left:
            return "left";
        case rive::TextAlign::right:
            return "right";
        case rive::TextAlign::center:
            return "center";
        default:
            return "left";
    }
}

static std::string verticalTextAlignToString(rive::VerticalTextAlign align)
{
    switch (align)
    {
        case rive::VerticalTextAlign::top:
            return "top";
        case rive::VerticalTextAlign::middle:
            return "middle";
        case rive::VerticalTextAlign::bottom:
            return "bottom";
        default:
            return "top";
    }
}

static std::string textSizingToString(rive::TextSizing sizing)
{
    switch (sizing)
    {
        case rive::TextSizing::autoWidth:
            return "autoWidth";
        case rive::TextSizing::autoHeight:
            return "autoHeight";
        case rive::TextSizing::fixed:
            return "fixed";
        default:
            return "autoWidth";
    }
}

static std::string textOverflowToString(rive::TextOverflow overflow)
{
    switch (overflow)
    {
        case rive::TextOverflow::visible:
            return "visible";
        case rive::TextOverflow::hidden:
            return "hidden";
        case rive::TextOverflow::clipped:
            return "clipped";
        case rive::TextOverflow::ellipsis:
            return "ellipsis";
        case rive::TextOverflow::fit:
            return "fit";
        default:
            return "visible";
    }
}

static std::string textWrapToString(rive::TextWrap wrap)
{
    switch (wrap)
    {
        case rive::TextWrap::wrap:
            return "wrap";
        case rive::TextWrap::noWrap:
            return "noWrap";
        default:
            return "wrap";
    }
}

// Display name + kind for a state-machine layer state.
static void describeLayerState(const rive::LayerState* state,
                               std::string& outName,
                               std::string& outType)
{
    if (state->is<rive::AnimationState>())
    {
        outType = "animation";
        const auto* animState = state->as<rive::AnimationState>();
        if (const auto* anim = animState->animation())
        {
            outName = anim->name();
        }
    }
    else if (state->is<rive::AnyState>())
    {
        outType = "any";
        outName = "any";
    }
    else if (state->is<rive::EntryState>())
    {
        outType = "entry";
        outName = "entry";
    }
    else if (state->is<rive::ExitState>())
    {
        outType = "exit";
        outName = "exit";
    }
    else
    {
        outType = "other";
    }
}

static rive::rcp<rive::File> openFile(const char name[],
                                     rive::ImportResult* importResult)
{
    FILE* f = fopen(name, "rb");
    if (!f)
    {
        return nullptr;
    }

    fseek(f, 0, SEEK_END);
    auto length = ftell(f);
    fseek(f, 0, SEEK_SET);

    std::vector<uint8_t> bytes(length);

    if (fread(bytes.data(), 1, length, f) != length)
    {
        fclose(f);
        printf("Failed to read file into bytes array\n");
        return nullptr;
    }
    fclose(f);

    static rive::NoOpFactory gFactory;
    return rive::File::import(bytes, &gFactory, importResult);
}

static std::vector<AnimationInfo> getAnimationsFromArtboard(
    rive::ArtboardInstance* artboard)
{
    std::vector<AnimationInfo> animations;
    auto animationCount = artboard->animationCount();
    for (int i = 0; i < animationCount; i++)
    {
        auto animationInstance = artboard->animationAt(i);
        AnimationInfo info;
        info.name = animationInstance->name();
        if (const auto* animation = animationInstance->animation())
        {
            info.fps = animation->fps();
            info.durationSeconds = animation->durationSeconds();
            info.loop = loopToString(animation->loop());
            info.speed = animation->speed();
        }
        animations.push_back(info);
    }
    return animations;
}

static std::vector<EventInfo> getEventsFromArtboard(
    rive::ArtboardInstance* artboard)
{
    std::vector<EventInfo> eventsInfo;
    std::vector<rive::Event*> events = artboard->find<rive::Event>();

    for (auto* event : events)
    {
        if (event->name().empty())
        {
            continue;
        }

        EventInfo info;
        info.name = event->name();

        if (event->is<rive::OpenUrlEvent>())
        {
            auto* openUrl = event->as<rive::OpenUrlEvent>();
            info.type = "openUrl";
            info.url = openUrl->url();
            info.target = std::to_string(openUrl->targetValue());
        }
        else if (event->is<rive::AudioEvent>())
        {
            auto* audio = event->as<rive::AudioEvent>();
            info.type = "audio";
            info.assetId = std::to_string(audio->assetId());
        }
        else
        {
            info.type = "general";
        }

        for (auto* prop : event->customProperties())
        {
            if (prop->name().empty())
            {
                continue;
            }
            EventPropertyInfo propInfo;
            propInfo.name = prop->name();

            switch (prop->coreType())
            {
                case rive::CustomPropertyNumberBase::typeKey:
                    propInfo.type = "number";
                    propInfo.defaultValue = formatNumber(
                        prop->as<rive::CustomPropertyNumber>()->propertyValue());
                    break;
                case rive::CustomPropertyBooleanBase::typeKey:
                    propInfo.type = "boolean";
                    propInfo.defaultValue =
                        prop->as<rive::CustomPropertyBoolean>()->propertyValue()
                            ? "true"
                            : "false";
                    break;
                case rive::CustomPropertyStringBase::typeKey:
                    propInfo.type = "string";
                    propInfo.defaultValue =
                        prop->as<rive::CustomPropertyString>()->propertyValue();
                    break;
                case rive::CustomPropertyColorBase::typeKey:
                    propInfo.type = "color";
                    propInfo.defaultValue = std::to_string(
                        prop->as<rive::CustomPropertyColor>()->propertyValue());
                    break;
                case rive::CustomPropertyEnumBase::typeKey:
                    propInfo.type = "enum";
                    propInfo.defaultValue = std::to_string(
                        prop->as<rive::CustomPropertyEnum>()->propertyValue());
                    break;
                case rive::CustomPropertyTriggerBase::typeKey:
                    propInfo.type = "trigger";
                    propInfo.defaultValue = "";
                    break;
                default:
                    propInfo.type = "unknown";
                    propInfo.defaultValue = "";
                    break;
            }
            info.properties.push_back(propInfo);
        }

        std::sort(info.properties.begin(), info.properties.end(),
                  [](const EventPropertyInfo& a, const EventPropertyInfo& b) {
                      return a.name < b.name;
                  });

        eventsInfo.push_back(info);
    }

    std::sort(eventsInfo.begin(), eventsInfo.end(),
              [](const EventInfo& a, const EventInfo& b) {
                  return a.name < b.name;
              });

    return eventsInfo;
}

static std::vector<StateMachineInfo>
getStateMachinesFromArtboard(rive::ArtboardInstance* artboard)
{
    std::vector<StateMachineInfo> stateMachines;
    auto stateMachineCount = artboard->stateMachineCount();
    for (size_t i = 0; i < stateMachineCount; i++)
    {
        auto stateMachine = artboard->stateMachineAt(i);
        StateMachineInfo smInfo;
        smInfo.name = stateMachine->name();

        auto inputCount = stateMachine->inputCount();
        for (size_t j = 0; j < inputCount; j++)
        {
            auto input = stateMachine->input(j);

            std::string inputType;
            std::string defaultValue;

            // Determine the input type and default value
            switch (input->inputCoreType())
            {
                case rive::StateMachineNumberBase::typeKey:
                {
                    auto smiNumberInput = static_cast<rive::SMINumber*>(input);
                    inputType = "number";
                    defaultValue = std::to_string(smiNumberInput->value());
                    break;
                }
                case rive::StateMachineTriggerBase::typeKey:
                {
                    inputType = "trigger";
                    defaultValue = "false";
                    break;
                }
                case rive::StateMachineBoolBase::typeKey:
                {
                    auto smiBoolInput = static_cast<rive::SMIBool*>(input);
                    inputType = "boolean";
                    defaultValue = smiBoolInput->value() ? "true" : "false";
                    break;
                }
                default:
                {
                    inputType = "unknown";
                    defaultValue = "";
                    break;
                }
            }

            smInfo.inputs.push_back({input->name(), inputType, defaultValue});
        }

        // States and transitions from the state machine definition.
        if (const auto* definition = artboard->stateMachine(i))
        {
            for (size_t l = 0; l < definition->layerCount(); l++)
            {
                const auto* layer = definition->layer(l);
                if (!layer)
                {
                    continue;
                }
                for (size_t s = 0; s < layer->stateCount(); s++)
                {
                    const auto* state = layer->state(s);
                    if (!state)
                    {
                        continue;
                    }
                    StateInfo stateInfo;
                    describeLayerState(state, stateInfo.name, stateInfo.type);

                    for (size_t t = 0; t < state->transitionCount(); t++)
                    {
                        const auto* transition = state->transition(t);
                        if (transition && transition->stateTo())
                        {
                            std::string toName, toType;
                            describeLayerState(transition->stateTo(),
                                               toName,
                                               toType);
                            if (!toName.empty())
                            {
                                stateInfo.transitions.push_back(toName);
                            }
                        }
                    }
                    smInfo.states.push_back(stateInfo);
                }
            }
        }

        stateMachines.push_back(smInfo);
    }
    return stateMachines;
}

static std::vector<std::string> findRiveFiles(const std::vector<std::string>& paths)
{
    std::vector<std::string> riveFile;

    for (const auto& path : paths)
    {
        if (std::filesystem::is_directory(path))
        {
            for (const auto& entry : std::filesystem::directory_iterator(path))
            {
                if (entry.path().extension() == ".riv")
                {
                    riveFile.push_back(entry.path().string());
                }
            }
        }
        else if (std::filesystem::path(path).extension() == ".riv")
        {
            riveFile.push_back(path);
        }
    }

    return riveFile;
}

static std::string makeUnique(const std::string& base,
                       std::unordered_set<std::string>& usedNames)
{
    std::string uniqueName = base;
    int counter = 1;
    while (usedNames.find(uniqueName) != usedNames.end())
    {
        uniqueName = base + "U" + std::to_string(counter);
        counter++;
    }
    usedNames.insert(uniqueName);
    return uniqueName;
}

template <typename T = rive::Component>
void findAll(std::vector<T*>& results, rive::ArtboardInstance* artboard)
{
    for (auto object : artboard->objects())
    {
        if (object != nullptr && object->is<T>())
        {
            results.push_back(static_cast<T*>(object));
        }
    }
}

static std::vector<TextValueRunInfo> getTextValueRunsFromArtboard(
    rive::ArtboardInstance* artboard)
{
    std::vector<rive::TextValueRun*> textValueRuns;
    std::vector<TextValueRunInfo> textValueRunsInfo;

    findAll<rive::TextValueRun>(textValueRuns, artboard);

    for (auto textValueRun : textValueRuns)
    {
        if (textValueRun->name().empty())
        {
            continue;
        }

        TextValueRunInfo info;
        info.name = textValueRun->name();
        info.defaultValue = textValueRun->text();

        if (auto* style = textValueRun->style())
        {
            info.fontSize = style->fontSize();
            info.lineHeight = style->lineHeight();
            info.letterSpacing = style->letterSpacing();
            info.fontAssetId = std::to_string(style->fontAssetId());
        }

        // Text-level alignment comes from the parent Text component.
        if (auto* parent = textValueRun->parent())
        {
            if (parent->is<rive::Text>())
            {
                auto* text = parent->as<rive::Text>();
                info.align = textAlignToString(text->align());
                info.verticalAlign =
                    verticalTextAlignToString(text->verticalAlign());
                info.sizing = textSizingToString(text->sizing());
                info.overflow = textOverflowToString(text->overflow());
                info.wrap = textWrapToString(text->wrap());
            }
        }

        textValueRunsInfo.push_back(info);
    }
    return textValueRunsInfo;
}

static std::vector<NestedTextValueRunInfo>
getNestedTextValueRunPathsFromArtboard(
    rive::ArtboardInstance* artboard,
    const std::string& currentPath = "")
{
    std::vector<NestedTextValueRunInfo> nestedTextValueRunsInfo;
    auto count = artboard->nestedArtboards().size();

    if (!currentPath.empty())
    {
        auto textRuns = getTextValueRunsFromArtboard(artboard);
        for (const auto& textRun : textRuns)
        {
            nestedTextValueRunsInfo.push_back(
                {textRun.name, currentPath});
        }
    }

    // Recursively process nested artboards
    for (int i = 0; i < count; i++)
    {
        auto nested = artboard->nestedArtboards()[i];
        auto nestedName = nested->name();
        if (!nestedName.empty())
        {
            // Only process nested artboards that have an exported name
            std::string newPath = currentPath.empty()
                                       ? nested->name()
                                       : currentPath + "/" + nested->name();

            auto nestedResults = getNestedTextValueRunPathsFromArtboard(
                nested->artboardInstance(),
                newPath);
            nestedTextValueRunsInfo.insert(
                nestedTextValueRunsInfo.end(),
                nestedResults.begin(),
                nestedResults.end());
        }
    }

    return nestedTextValueRunsInfo;
}

static std::vector<AssetInfo> getAssetsFromFile(rive::File* file)
{
    std::vector<AssetInfo> assetsInfo;
    std::unordered_set<std::string> usedAssetNames;

    auto assets = file->assets();
    for (auto asset : assets)
    {
        std::string assetType;
        switch (asset->coreType())
        {
            case rive::ImageAsset::typeKey:
                assetType = "image";
                break;
            case rive::FontAsset::typeKey:
                assetType = "font";
                break;
            case rive::AudioAsset::typeKey:
                assetType = "audio";
                break;
            default:
                assetType = "unknown";
                break;
        }

        auto assetName = asset->name();
        auto uniqueAssetName = makeUnique(assetName, usedAssetNames);

        AssetInfo info;
        info.name = uniqueAssetName;
        info.type = assetType;
        info.fileExtension = asset->fileExtension();
        info.assetId = std::to_string(asset->assetId());
        info.cdnUuid = asset->cdnUuidStr();
        info.cdnBaseUrl = asset->cdnBaseUrl();
        info.uniqueFilename = asset->uniqueFilename();
        // Embedded assets have no CDN UUID; CDN-referenced ones do.
        info.isEmbedded = asset->cdnUuidStr().empty();

        if (asset->coreType() == rive::ImageAsset::typeKey)
        {
            auto* image = static_cast<rive::ImageAsset*>(asset.get());
            info.width = image->width();
            info.height = image->height();
        }

        assetsInfo.push_back(info);
    }
    return assetsInfo;
}

static std::string dataTypeToString(rive::DataType type)
{
    switch (type)
    {
        case rive::DataType::none:
            return "none";
        case rive::DataType::string:
            return "string";
        case rive::DataType::number:
            return "number";
        case rive::DataType::boolean:
            return "boolean";
        case rive::DataType::color:
            return "color";
        case rive::DataType::list:
            return "list";
        case rive::DataType::enumType:
            return "enum";
        case rive::DataType::trigger:
            return "trigger";
        case rive::DataType::viewModel:
            return "viewModel";
        case rive::DataType::integer:
            return "integer";
        case rive::DataType::symbolListIndex:
            return "symbolListIndex";
        case rive::DataType::assetImage:
            return "assetImage";
        default:
            return "unknown";
    }
}

static std::optional<RiveFileData> processRiveFile(const std::string& riveFilePath)
{
    // Check if the file is empty
    if (std::filesystem::is_empty(riveFilePath))
    {
        console::error("Rive file is empty: " + riveFilePath);
        return std::nullopt;
    }

    rive::ImportResult importResult = rive::ImportResult::malformed;
    auto riveFile = openFile(riveFilePath.c_str(), &importResult);
    if (!riveFile)
    {
        switch (importResult)
        {
            case rive::ImportResult::unsupportedVersion:
                console::error(
                    "Unsupported Rive runtime version (generator supports up "
                    "to " +
                    std::to_string(rive::File::majorVersion) + "." +
                    std::to_string(rive::File::minorVersion) + "): " +
                    riveFilePath);
                break;
            case rive::ImportResult::malformed:
                console::error("Malformed Rive file: " + riveFilePath);
                break;
            default:
                console::error("Failed to parse Rive file: " + riveFilePath);
                break;
        }
        return std::nullopt;
    }

    std::filesystem::path path(riveFilePath);
    std::string fileNameWithoutExtension = path.stem().string();
    std::vector<AssetInfo> assets = getAssetsFromFile(riveFile.get());
    RiveFileData fileData;
    fileData.rivName = fileNameWithoutExtension;
    fileData.rivPascalCase = toPascalCase(fileNameWithoutExtension);
    fileData.rivCameCase = toCamelCase(fileNameWithoutExtension);
    fileData.riveSnakeCase = toSnakeCase(fileNameWithoutExtension);
    fileData.rivKebabCase = toKebabCase(fileNameWithoutExtension);
    fileData.assets = assets;

    // Process enums
    const auto& fileEnums = riveFile->enums();
    for (auto* dataEnum : fileEnums)
    {
        if (dataEnum)
        {
            EnumInfo enumInfo;
            enumInfo.name = dataEnum->enumName();
            const auto& values = dataEnum->values();
            for (const auto* value : values)
            {
                enumInfo.values.push_back({value->key(), value->value()});
            }
            fileData.enums.push_back(enumInfo);
        }
    }

    // Process view models
    for (size_t i = 0; i < riveFile->viewModelCount(); i++)
    {
        auto viewModel = riveFile->viewModelByIndex(i);
        if (viewModel)
        {
            ViewModelInfo viewModelInfo;
            viewModelInfo.name = viewModel->name();
            viewModelInfo.instanceNames = viewModel->instanceNames();
            std::sort(viewModelInfo.instanceNames.begin(),
                      viewModelInfo.instanceNames.end());
            // Resolve the view model definition for clean reference lookups
            // (nested view models / enums) and a default instance for reading
            // default property values via the runtime API.
            rive::ViewModel* vmDef = riveFile->viewModel(viewModel->name());
            auto defaultInstance = viewModel->createDefaultInstance();

            auto readDefault = [&](const std::string& name,
                                   const std::string& type,
                                   PropertyInfo& pi) {
                if (!defaultInstance)
                {
                    return;
                }
                if (type == "number")
                {
                    if (auto* v = defaultInstance->propertyNumber(name))
                    {
                        pi.defaultValue = formatNumber(v->value());
                        pi.hasDefaultValue = true;
                    }
                }
                else if (type == "string")
                {
                    if (auto* v = defaultInstance->propertyString(name))
                    {
                        pi.defaultValue = v->value();
                        pi.hasDefaultValue = true;
                    }
                }
                else if (type == "boolean")
                {
                    if (auto* v = defaultInstance->propertyBoolean(name))
                    {
                        pi.defaultValue = v->value() ? "true" : "false";
                        pi.hasDefaultValue = true;
                    }
                }
                else if (type == "color")
                {
                    if (auto* v = defaultInstance->propertyColor(name))
                    {
                        pi.defaultValue = std::to_string(v->value());
                        pi.hasDefaultValue = true;
                    }
                }
                else if (type == "enum")
                {
                    if (auto* v = defaultInstance->propertyEnum(name))
                    {
                        pi.defaultValue = v->value();
                        pi.hasDefaultValue = true;
                    }
                }
            };

            auto propertiesData = viewModel->properties();
            for (const auto& property : propertiesData)
            {
                PropertyInfo pi;
                pi.name = property.name;
                pi.type = dataTypeToString(property.type);

                if (property.type == rive::DataType::viewModel && vmDef)
                {
                    if (auto* prop = vmDef->property(property.name))
                    {
                        if (prop->is<rive::ViewModelPropertyViewModel>())
                        {
                            auto refId =
                                prop->as<rive::ViewModelPropertyViewModel>()
                                    ->viewModelReferenceId();
                            if (auto* refVm = riveFile->viewModel(refId))
                            {
                                pi.backingName = refVm->name();
                            }
                        }
                    }
                }
                else if (property.type == rive::DataType::enumType && vmDef)
                {
                    if (auto* prop = vmDef->property(property.name))
                    {
                        if (prop->is<rive::ViewModelPropertyEnum>())
                        {
                            if (auto* de =
                                    prop->as<rive::ViewModelPropertyEnum>()
                                        ->dataEnum())
                            {
                                pi.backingName = de->enumName();
                            }
                        }
                    }
                    readDefault(property.name, pi.type, pi);
                }
                else if (property.type == rive::DataType::list)
                {
                    // The item view-model type is not stored on the property
                    // definition; best-effort read it from the default
                    // instance's list items (empty by default => unknown).
                    if (defaultInstance)
                    {
                        if (auto* list =
                                defaultInstance->propertyList(property.name))
                        {
                            if (list->size() > 0)
                            {
                                if (auto item = list->instanceAt(0))
                                {
                                    pi.backingName = item->viewModelName();
                                }
                            }
                        }
                    }
                }
                else
                {
                    readDefault(property.name, pi.type, pi);
                }

                viewModelInfo.properties.push_back(pi);
            }
            fileData.viewmodels.push_back(viewModelInfo);
        }
    }

    std::unordered_set<std::string> usedArtboardNames;

    auto artboardCount = riveFile->artboardCount();
    for (int i = 0; i < artboardCount; i++)
    {
        auto artboard = riveFile->artboardAt(i);
        std::string artboardName = artboard->name();

        std::string artboardPascalCase = toPascalCase(artboardName);
        std::string artboardCameCase = toCamelCase(artboardName);
        std::string artboardSnakeCase = toSnakeCase(artboardName);
        std::string artboardKebabCase = toKebabCase(artboardName);

        // Ensure unique artboard variable names
        artboardCameCase =
            makeUnique(artboardCameCase, usedArtboardNames);

        std::vector<AnimationInfo> animations =
            getAnimationsFromArtboard(artboard.get());
        std::vector<StateMachineInfo> stateMachines =
            getStateMachinesFromArtboard(artboard.get());
        std::vector<TextValueRunInfo> textValueRuns =
            getTextValueRunsFromArtboard(artboard.get());
        std::vector<NestedTextValueRunInfo> nestedTextValueRuns =
            getNestedTextValueRunPathsFromArtboard(artboard.get());
        std::vector<EventInfo> events = getEventsFromArtboard(artboard.get());

        ArtboardData artboardData;
        artboardData.artboardName = artboardName;
        artboardData.artboardPascalCase = artboardPascalCase;
        artboardData.artboardCameCase = artboardCameCase;
        artboardData.artboardSnakeCase = artboardSnakeCase;
        artboardData.artboardKebabCase = artboardKebabCase;
        artboardData.animations = animations;
        artboardData.stateMachines = stateMachines;
        artboardData.textValueRuns = textValueRuns;
        artboardData.nestedTextValueRuns = nestedTextValueRuns;
        artboardData.events = events;
        artboardData.width = artboard->originalWidth();
        artboardData.height = artboard->originalHeight();
        artboardData.originX = artboard->originX();
        artboardData.originY = artboard->originY();
        artboardData.clip = artboard->clip();

        // Default state machine (defaultStateMachineId is an index into the
        // artboard's state machines; unset values fall outside the range).
        auto defaultSmId = artboard->defaultStateMachineId();
        if (defaultSmId < artboard->stateMachineCount())
        {
            if (auto sm = artboard->stateMachineAt(defaultSmId))
            {
                artboardData.hasDefaultStateMachine = true;
                artboardData.defaultStateMachineName = sm->name();
            }
        }

        // Bound (default) view model for this artboard, if any.
        if (auto* boundVm =
                riveFile->defaultArtboardViewModel(artboard.get()))
        {
            artboardData.hasBoundViewModel = true;
            artboardData.boundViewModelName = boundVm->name();
        }

        fileData.artboards.push_back(artboardData);
    }

    // Sort all collections alphabetically for deterministic output
    std::sort(fileData.assets.begin(), fileData.assets.end(),
              [](const AssetInfo& a, const AssetInfo& b) {
                  return a.name < b.name;
              });

    std::sort(fileData.enums.begin(), fileData.enums.end(),
              [](const EnumInfo& a, const EnumInfo& b) {
                  return a.name < b.name;
              });
    for (auto& enumInfo : fileData.enums)
    {
        std::sort(enumInfo.values.begin(), enumInfo.values.end(),
                  [](const EnumValueInfo& a, const EnumValueInfo& b) {
                      return a.key < b.key;
                  });
    }

    std::sort(fileData.viewmodels.begin(), fileData.viewmodels.end(),
              [](const ViewModelInfo& a, const ViewModelInfo& b) {
                  return a.name < b.name;
              });
    for (auto& vm : fileData.viewmodels)
    {
        std::sort(vm.properties.begin(), vm.properties.end(),
                  [](const PropertyInfo& a, const PropertyInfo& b) {
                      return a.name < b.name;
                  });
    }

    std::sort(fileData.artboards.begin(), fileData.artboards.end(),
              [](const ArtboardData& a, const ArtboardData& b) {
                  return a.artboardName < b.artboardName;
              });
    for (auto& artboard : fileData.artboards)
    {
        std::sort(artboard.animations.begin(), artboard.animations.end(),
                  [](const AnimationInfo& a, const AnimationInfo& b) {
                      return a.name < b.name;
                  });

        std::sort(artboard.stateMachines.begin(), artboard.stateMachines.end(),
                  [](const StateMachineInfo& a, const StateMachineInfo& b) {
                      return a.name < b.name;
                  });
        for (auto& sm : artboard.stateMachines)
        {
            std::sort(sm.inputs.begin(), sm.inputs.end(),
                      [](const InputInfo& a, const InputInfo& b) {
                          return a.name < b.name;
                      });
            std::sort(sm.states.begin(), sm.states.end(),
                      [](const StateInfo& a, const StateInfo& b) {
                          return a.name < b.name;
                      });
        }

        std::sort(artboard.textValueRuns.begin(), artboard.textValueRuns.end(),
                  [](const TextValueRunInfo& a, const TextValueRunInfo& b) {
                      return a.name < b.name;
                  });

        std::sort(artboard.nestedTextValueRuns.begin(),
                  artboard.nestedTextValueRuns.end(),
                  [](const NestedTextValueRunInfo& a,
                     const NestedTextValueRunInfo& b) {
                      return a.name < b.name;
                  });
    }

    return fileData;
}

static std::optional<std::string> readTemplateFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        console::warning("Unable to open template file: " + path);
        return std::nullopt;
    }
    return std::string(std::istreambuf_iterator<char>(file),
                       std::istreambuf_iterator<char>());
}

int main(int argc, char* argv[])
{
    CLI::App app{"Rive Code Generator"};

    std::vector<std::string> inputPaths;
    std::string outputFilePath;
    std::string templatePath;
    Language language = Language::Dart; // Default to Dart

    app.add_option("-i, --input",
                   inputPaths,
                   "Path to Rive file or directory containing Rive files")
        ->required()
        ->check(CLI::ExistingFile | CLI::ExistingDirectory);

    app.add_option("-o, --output", outputFilePath, "Output file path")
        ->required();

    app.add_option("-t,--template", templatePath, "Custom template file path");

    app.add_option("-l, --language",
                   language,
                   "Programming language for code generation")
        ->transform(CLI::CheckedTransformer(
            std::map<std::string, Language>{{"dart", Language::Dart},
                                            {"js", Language::JavaScript}},
            CLI::ignore_case));

    CLI11_PARSE(app, argc, argv)

    console::header("Rive Code Generator");
    console::Timer timer;
    console::Stats stats;

    std::string templateStr;
    if (!templatePath.empty())
    {
        auto customTemplate = readTemplateFile(templatePath);
        if (customTemplate)
        {
            templateStr = *customTemplate;
            console::success("Template loaded (custom: " + templatePath + ")");
        }
        else
        {
            console::warning("Custom template failed, using default dart");
            templateStr = default_templates::DEFAULT_DART_TEMPLATE;
        }
    }
    else
    {
        if (language == Language::Dart)
        {
            templateStr = default_templates::DEFAULT_DART_TEMPLATE;
            console::success("Template loaded (default dart)");
        }
        else if (language == Language::JavaScript)
        {
            console::error("JavaScript code generation is not yet supported");
            return 1;
        }
    }

    std::vector<std::string> riveFiles = findRiveFiles(inputPaths);

    if (riveFiles.empty())
    {
        console::error("No .riv files found in the specified path");
        return 1;
    }

    console::success("Found " + console::pluralize((int)riveFiles.size(), "file"));
    console::blank();
    std::cout << "  Processing files..." << std::endl;

    std::vector<RiveFileData> riveFileDataList;
    for (const auto& riv_file : riveFiles)
    {
        // Print the file name BEFORE parsing so any warnings emitted by the
        // runtime importer (e.g. duplicate asset content, written to stderr)
        // are clearly attributed to the file currently being processed.
        std::filesystem::path p(riv_file);
        console::step(p.filename().string());

        auto result = processRiveFile(riv_file);
        if (result)
        {
            riveFileDataList.push_back(*result);

            const auto& fd = *result;

            int numArtboards = (int)fd.artboards.size();
            int numAnimations = 0;
            int numStateMachines = 0;
            for (const auto& ab : fd.artboards)
            {
                numAnimations += (int)ab.animations.size();
                numStateMachines += (int)ab.stateMachines.size();
            }
            console::detail(console::pluralize(numArtboards, "artboard") +
                            ", " +
                            console::pluralize(numAnimations, "animation") +
                            ", " +
                            console::pluralize(numStateMachines, "state machine"));

            int numAssets = (int)fd.assets.size();
            if (numAssets > 0)
            {
                int images = 0, fonts = 0, audio = 0, unknown = 0;
                for (const auto& a : fd.assets)
                {
                    if (a.type == "image") images++;
                    else if (a.type == "font") fonts++;
                    else if (a.type == "audio") audio++;
                    else unknown++;
                }
                console::detail(console::pluralize(numAssets, "asset") +
                                " (" + console::assetBreakdown(images, fonts, audio, unknown) + ")");
            }

            stats.files++;
            stats.artboards += numArtboards;
            stats.animations += numAnimations;
            stats.stateMachines += numStateMachines;
            stats.assets += numAssets;
            stats.enums += (int)fd.enums.size();
            stats.viewModels += (int)fd.viewmodels.size();
        }
        else
        {
            stats.errors++;
        }
    }

    // Mustache template rendering
    kainjow::mustache::data templateData;
    std::vector<kainjow::mustache::data> riveFileList;

    for (size_t fileIndex = 0; fileIndex < riveFileDataList.size();
         fileIndex++)
    {
        const auto& fileData = riveFileDataList[fileIndex];
        kainjow::mustache::data riveFileData;
        riveFileData["riv_name"] = fileData.rivName;
        riveFileData["riv_pascal_case"] = fileData.rivPascalCase;
        riveFileData["riv_camel_case"] = fileData.rivCameCase;
        riveFileData["riv_snake_case"] = fileData.riveSnakeCase;
        riveFileData["riv_kebab_case"] = fileData.rivKebabCase;
        riveFileData["last"] = (fileIndex == riveFileDataList.size() - 1);

        // Add enums to template data
        std::vector<kainjow::mustache::data> enums;
        for (size_t enumIndex = 0; enumIndex < fileData.enums.size();
             enumIndex++)
        {
            const auto& enumInfo = fileData.enums[enumIndex];
            kainjow::mustache::data enumData;
            enumData["enum_name"] = enumInfo.name;
            enumData["enum_camel_case"] = toCamelCase(enumInfo.name);
            enumData["enum_pascal_case"] = toPascalCase(enumInfo.name);
            enumData["enum_snake_case"] = toSnakeCase(enumInfo.name);
            enumData["enum_kebab_case"] = toKebabCase(enumInfo.name);
            enumData["last"] = (enumIndex == fileData.enums.size() - 1);

            std::vector<kainjow::mustache::data> enumValues;
            for (size_t valueIndex = 0; valueIndex < enumInfo.values.size();
                 valueIndex++)
            {
                const auto& value = enumInfo.values[valueIndex];
                kainjow::mustache::data valueData;
                valueData["enum_value_key"] = value.key;
                valueData["enum_value_value"] = value.value;
                valueData["enum_value_camel_case"] = toCamelCase(value.key);
                valueData["enum_value_pascal_case"] = toPascalCase(value.key);
                valueData["enum_value_snake_case"] = toSnakeCase(value.key);
                valueData["enum_value_kebab_case"] = toKebabCase(value.key);
                valueData["last"] =
                    (valueIndex == enumInfo.values.size() - 1);
                enumValues.push_back(valueData);
            }
            enumData["enum_values"] = enumValues;
            enums.push_back(enumData);
        }
        riveFileData["enums"] = enums;

        // Add view models to template data
        std::vector<kainjow::mustache::data> viewmodels;
        for (size_t vmIndex = 0; vmIndex < fileData.viewmodels.size();
             vmIndex++)
        {
            const auto& viewModel = fileData.viewmodels[vmIndex];
            kainjow::mustache::data viewmodelData;
            viewmodelData["view_model_name"] = viewModel.name;
            viewmodelData["view_model_camel_case"] =
                toCamelCase(viewModel.name);
            viewmodelData["view_model_pascal_case"] =
                toPascalCase(viewModel.name);
            viewmodelData["view_model_snake_case"] =
                toSnakeCase(viewModel.name);
            viewmodelData["view_model_kebab_case"] =
                toKebabCase(viewModel.name);
            viewmodelData["last"] =
                (vmIndex == fileData.viewmodels.size() - 1);

            std::vector<kainjow::mustache::data> instanceNames;
            for (size_t instIndex = 0;
                 instIndex < viewModel.instanceNames.size();
                 instIndex++)
            {
                const auto& instName = viewModel.instanceNames[instIndex];
                kainjow::mustache::data instData;
                instData["instance_name"] = instName;
                instData["instance_camel_case"] = toCamelCase(instName);
                instData["instance_pascal_case"] = toPascalCase(instName);
                instData["last"] =
                    (instIndex == viewModel.instanceNames.size() - 1);
                instanceNames.push_back(instData);
            }
            viewmodelData["instance_names"] = instanceNames;

            std::vector<kainjow::mustache::data> properties;
            for (size_t propIndex = 0;
                 propIndex < viewModel.properties.size();
                 propIndex++)
            {
                const auto& property = viewModel.properties[propIndex];
                kainjow::mustache::data propertyData;
                propertyData["property_name"] = property.name;
                propertyData["property_camel_case"] =
                    toCamelCase(property.name);
                propertyData["property_pascal_case"] =
                    toPascalCase(property.name);
                propertyData["property_snake_case"] =
                    toSnakeCase(property.name);
                propertyData["property_kebab_case"] =
                    toKebabCase(property.name);
                propertyData["property_default_value"] = property.defaultValue;
                propertyData.set("has_default_value", property.hasDefaultValue);

                // Add property type information for the viewmodel template
                kainjow::mustache::data propertyTypeData;
                propertyTypeData.set("type_name", property.type);
                propertyTypeData.set("is_view_model",
                                       property.type == "viewModel");
                propertyTypeData.set("is_enum", property.type == "enum");
                propertyTypeData.set("is_string", property.type == "string");
                propertyTypeData.set("is_number", property.type == "number");
                propertyTypeData.set("is_integer",
                                       property.type == "integer");
                propertyTypeData.set("is_boolean",
                                       property.type == "boolean");
                propertyTypeData.set("is_color", property.type == "color");
                propertyTypeData.set("is_list", property.type == "list");
                propertyTypeData.set("is_symbol_list_index",
                                       property.type == "symbolListIndex");
                propertyTypeData.set("is_asset_image",
                                       property.type == "assetImage");
                propertyTypeData.set("is_trigger",
                                       property.type == "trigger");
                propertyTypeData.set("backing_name", property.backingName);
                propertyTypeData.set("backing_camel_case",
                                       toCamelCase(property.backingName));
                propertyTypeData.set("backing_pascal_case",
                                       toPascalCase(property.backingName));
                propertyTypeData.set("backing_snake_case",
                                       toSnakeCase(property.backingName));
                propertyTypeData.set("backing_kebab_case",
                                       toKebabCase(property.backingName));
                propertyData.set("property_type", propertyTypeData);

                propertyData["last"] =
                    (propIndex == viewModel.properties.size() - 1);
                properties.push_back(propertyData);
            }
            viewmodelData["properties"] = properties;
            viewmodels.push_back(viewmodelData);
        }
        riveFileData["view_models"] = viewmodels;

        std::vector<kainjow::mustache::data> assets;
        for (size_t assetIndex = 0; assetIndex < fileData.assets.size();
             assetIndex++)
        {
            const auto& asset = fileData.assets[assetIndex];
            kainjow::mustache::data assetData;
            assetData["asset_name"] = asset.name;
            assetData["asset_camel_case"] = toCamelCase(asset.name);
            assetData["asset_pascal_case"] = toPascalCase(asset.name);
            assetData["asset_snake_case"] = toSnakeCase(asset.name);
            assetData["asset_kebab_case"] = toKebabCase(asset.name);
            assetData["asset_type"] = asset.type;
            assetData["asset_id"] = asset.assetId;
            assetData["asset_cdn_uuid"] = asset.cdnUuid;
            assetData["asset_cdn_base_url"] = asset.cdnBaseUrl;
            assetData["asset_unique_filename"] = asset.uniqueFilename;
            assetData.set("asset_is_embedded", asset.isEmbedded);
            assetData["asset_width"] = formatNumber(asset.width);
            assetData["asset_height"] = formatNumber(asset.height);
            assetData["last"] = (assetIndex == fileData.assets.size() - 1);
            assets.push_back(assetData);
        }
        riveFileData["assets"] = assets;

        std::vector<kainjow::mustache::data> artboardList;
        for (size_t artboardIndex = 0;
             artboardIndex < fileData.artboards.size();
             artboardIndex++)
        {
            const auto& artboard = fileData.artboards[artboardIndex];
            kainjow::mustache::data artboardData;
            artboardData["artboard_name"] = artboard.artboardName;
            artboardData["artboard_pascal_case"] =
                artboard.artboardPascalCase;
            artboardData["artboard_camel_case"] = artboard.artboardCameCase;
            artboardData["artboard_snake_case"] = artboard.artboardSnakeCase;
            artboardData["artboard_kebab_case"] = artboard.artboardKebabCase;
            artboardData["artboard_width"] = formatNumber(artboard.width);
            artboardData["artboard_height"] = formatNumber(artboard.height);
            artboardData["artboard_origin_x"] = formatNumber(artboard.originX);
            artboardData["artboard_origin_y"] = formatNumber(artboard.originY);
            artboardData.set("artboard_clip", artboard.clip);

            artboardData.set("has_default_state_machine",
                             artboard.hasDefaultStateMachine);
            artboardData["default_state_machine_name"] =
                artboard.defaultStateMachineName;
            artboardData["default_state_machine_camel_case"] =
                toCamelCase(artboard.defaultStateMachineName);
            artboardData["default_state_machine_pascal_case"] =
                toPascalCase(artboard.defaultStateMachineName);

            artboardData.set("has_bound_view_model",
                             artboard.hasBoundViewModel);
            artboardData["bound_view_model_name"] =
                artboard.boundViewModelName;
            artboardData["bound_view_model_camel_case"] =
                toCamelCase(artboard.boundViewModelName);
            artboardData["bound_view_model_pascal_case"] =
                toPascalCase(artboard.boundViewModelName);

            artboardData["last"] =
                (artboardIndex == fileData.artboards.size() - 1);

            std::unordered_set<std::string> usedAnimationNames;
            std::vector<kainjow::mustache::data> animations;
            for (size_t animIndex = 0; animIndex < artboard.animations.size();
                 animIndex++)
            {
                const auto& animation = artboard.animations[animIndex];
                kainjow::mustache::data animData;
                auto uniqueName = makeUnique(animation.name, usedAnimationNames);
                animData["animation_name"] = animation.name;
                animData["animation_camel_case"] = toCamelCase(uniqueName);
                animData["animation_pascal_case"] = toPascalCase(uniqueName);
                animData["animation_snake_case"] = toSnakeCase(uniqueName);
                animData["animation_kebab_case"] = toKebabCase(uniqueName);
                animData["animation_fps"] = std::to_string(animation.fps);
                animData["animation_duration_seconds"] =
                    formatNumber(animation.durationSeconds);
                animData["animation_loop"] = animation.loop;
                animData["animation_speed"] = formatNumber(animation.speed);
                animData["last"] =
                    (animIndex == artboard.animations.size() - 1);
                animations.push_back(animData);
            }
            artboardData["animations"] = animations;

            std::unordered_set<std::string> usedStateMachineNames;
            std::vector<kainjow::mustache::data> stateMachines;
            for (size_t smIndex = 0; smIndex < artboard.stateMachines.size();
                 smIndex++)
            {
                const auto& stateMachine = artboard.stateMachines[smIndex];
                kainjow::mustache::data stateMachineData;
                auto uniqueName =
                    makeUnique(stateMachine.name, usedStateMachineNames);
                stateMachineData["state_machine_name"] = stateMachine.name;
                stateMachineData["state_machine_camel_case"] =
                    toCamelCase(uniqueName);
                stateMachineData["state_machine_pascal_case"] =
                    toPascalCase(uniqueName);
                stateMachineData["state_machine_snake_case"] =
                    toSnakeCase(uniqueName);
                stateMachineData["state_machine_kebab_case"] =
                    toKebabCase(uniqueName);
                stateMachineData["last"] =
                    (smIndex == artboard.stateMachines.size() - 1);

                std::unordered_set<std::string> usedInputNames;
                std::vector<kainjow::mustache::data> inputs;
                for (size_t inputIndex = 0;
                     inputIndex < stateMachine.inputs.size();
                     inputIndex++)
                {
                    const auto& input = stateMachine.inputs[inputIndex];
                    kainjow::mustache::data inputData;
                    auto uniqueInputName =
                        makeUnique(input.name, usedInputNames);
                    inputData["input_name"] = input.name;
                    inputData["input_camel_case"] = toCamelCase(uniqueInputName);
                    inputData["input_pascal_case"] =
                        toPascalCase(uniqueInputName);
                    inputData["input_snake_case"] =
                        toSnakeCase(uniqueInputName);
                    inputData["input_kebab_case"] =
                        toKebabCase(uniqueInputName);
                    inputData["input_type"] = input.type;
                    inputData["input_default_value"] = input.defaultValue;
                    inputData["last"] =
                        (inputIndex == stateMachine.inputs.size() - 1);
                    inputs.push_back(inputData);
                }
                stateMachineData["inputs"] = inputs;

                std::unordered_set<std::string> usedStateNames;
                std::vector<kainjow::mustache::data> states;
                for (size_t stateIndex = 0;
                     stateIndex < stateMachine.states.size();
                     stateIndex++)
                {
                    const auto& state = stateMachine.states[stateIndex];
                    kainjow::mustache::data stateData;
                    auto uniqueStateName =
                        makeUnique(toCamelCase(state.name), usedStateNames);
                    stateData["state_name"] = state.name;
                    stateData["state_camel_case"] = uniqueStateName;
                    stateData["state_pascal_case"] = toPascalCase(state.name);
                    stateData["state_type"] = state.type;

                    std::vector<kainjow::mustache::data> transitions;
                    for (size_t trIndex = 0;
                         trIndex < state.transitions.size();
                         trIndex++)
                    {
                        kainjow::mustache::data trData;
                        trData["transition_to"] = state.transitions[trIndex];
                        trData["last"] =
                            (trIndex == state.transitions.size() - 1);
                        transitions.push_back(trData);
                    }
                    stateData["transitions"] = transitions;
                    stateData["last"] =
                        (stateIndex == stateMachine.states.size() - 1);
                    states.push_back(stateData);
                }
                stateMachineData["states"] = states;

                stateMachines.push_back(stateMachineData);
            }
            artboardData["state_machines"] = stateMachines;

            std::unordered_set<std::string> usedTextValueRunNames;
            std::vector<kainjow::mustache::data> textValueRuns;
            for (size_t tvrIndex = 0;
                 tvrIndex < artboard.textValueRuns.size();
                 tvrIndex++)
            {
                const auto& tvr = artboard.textValueRuns[tvrIndex];
                kainjow::mustache::data tvrData;
                auto uniqueName = makeUnique(tvr.name, usedTextValueRunNames);
                tvrData["text_value_run_name"] = tvr.name;
                tvrData["text_value_run_camel_case"] =
                    toCamelCase(uniqueName);
                tvrData["text_value_run_pascal_case"] =
                    toPascalCase(uniqueName);
                tvrData["text_value_run_snake_case"] =
                    toSnakeCase(uniqueName);
                tvrData["text_value_run_kebab_case"] =
                    toKebabCase(uniqueName);
                tvrData["text_value_run_default"] = tvr.defaultValue;
                tvrData["text_value_run_default_sanitized"] =
                    sanitizeString(tvr.defaultValue);
                tvrData["text_value_run_font_size"] =
                    formatNumber(tvr.fontSize);
                tvrData["text_value_run_line_height"] =
                    formatNumber(tvr.lineHeight);
                tvrData["text_value_run_letter_spacing"] =
                    formatNumber(tvr.letterSpacing);
                tvrData["text_value_run_font_asset_id"] = tvr.fontAssetId;
                tvrData["text_value_run_align"] = tvr.align;
                tvrData["text_value_run_vertical_align"] = tvr.verticalAlign;
                tvrData["text_value_run_sizing"] = tvr.sizing;
                tvrData["text_value_run_overflow"] = tvr.overflow;
                tvrData["text_value_run_wrap"] = tvr.wrap;
                tvrData["last"] =
                    (tvrIndex == artboard.textValueRuns.size() - 1);
                textValueRuns.push_back(tvrData);
            }
            artboardData["text_value_runs"] = textValueRuns;

            std::vector<kainjow::mustache::data> nestedTextValueRuns;
            for (size_t ntvrIndex = 0;
                 ntvrIndex < artboard.nestedTextValueRuns.size();
                 ntvrIndex++)
            {
                const auto& ntvr = artboard.nestedTextValueRuns[ntvrIndex];
                kainjow::mustache::data ntvrData;
                ntvrData["nested_text_value_run_name"] = ntvr.name;
                ntvrData["nested_text_value_run_path"] = ntvr.path;
                ntvrData["last"] =
                    (ntvrIndex == artboard.nestedTextValueRuns.size() - 1);
                nestedTextValueRuns.push_back(ntvrData);
            }

            artboardData["nested_text_value_runs"] = nestedTextValueRuns;

            std::unordered_set<std::string> usedEventNames;
            std::vector<kainjow::mustache::data> eventList;
            for (size_t eventIndex = 0; eventIndex < artboard.events.size();
                 eventIndex++)
            {
                const auto& event = artboard.events[eventIndex];
                kainjow::mustache::data eventData;
                auto uniqueName = makeUnique(event.name, usedEventNames);
                eventData["event_name"] = event.name;
                eventData["event_camel_case"] = toCamelCase(uniqueName);
                eventData["event_pascal_case"] = toPascalCase(uniqueName);
                eventData["event_snake_case"] = toSnakeCase(uniqueName);
                eventData["event_kebab_case"] = toKebabCase(uniqueName);
                eventData["event_type"] = event.type;
                eventData.set("is_general", event.type == "general");
                eventData.set("is_open_url", event.type == "openUrl");
                eventData.set("is_audio", event.type == "audio");
                eventData["event_url"] = event.url;
                eventData["event_target"] = event.target;
                eventData["event_asset_id"] = event.assetId;

                std::unordered_set<std::string> usedEventPropNames;
                std::vector<kainjow::mustache::data> eventProps;
                for (size_t propIndex = 0;
                     propIndex < event.properties.size();
                     propIndex++)
                {
                    const auto& prop = event.properties[propIndex];
                    kainjow::mustache::data propData;
                    auto uniquePropName =
                        makeUnique(prop.name, usedEventPropNames);
                    propData["property_name"] = prop.name;
                    propData["property_camel_case"] =
                        toCamelCase(uniquePropName);
                    propData["property_pascal_case"] =
                        toPascalCase(uniquePropName);
                    propData["property_snake_case"] =
                        toSnakeCase(uniquePropName);
                    propData["property_kebab_case"] =
                        toKebabCase(uniquePropName);
                    propData["property_type"] = prop.type;
                    propData["property_default_value"] = prop.defaultValue;
                    propData.set("is_number", prop.type == "number");
                    propData.set("is_boolean", prop.type == "boolean");
                    propData.set("is_string", prop.type == "string");
                    propData.set("is_color", prop.type == "color");
                    propData.set("is_enum", prop.type == "enum");
                    propData.set("is_trigger", prop.type == "trigger");
                    propData["last"] =
                        (propIndex == event.properties.size() - 1);
                    eventProps.push_back(propData);
                }
                eventData["properties"] = eventProps;
                eventData["last"] =
                    (eventIndex == artboard.events.size() - 1);
                eventList.push_back(eventData);
            }
            artboardData["events"] = eventList;

            artboardList.push_back(artboardData);
        }

        riveFileData["artboards"] = artboardList;
        riveFileList.push_back(riveFileData);
    }

    templateData["generated_file_name"] = generatedFileName;
    templateData["runtime_major_version"] =
        std::to_string(rive::File::majorVersion);
    templateData["runtime_minor_version"] =
        std::to_string(rive::File::minorVersion);
    templateData["riv_files"] = riveFileList;

    kainjow::mustache::mustache tmpl(templateStr);
    std::string result = tmpl.render(templateData);

    console::blank();
    console::success("Template rendered");

    std::filesystem::path output_path(outputFilePath);

    // If only a filename is provided, use the current directory
    if (output_path.is_relative() && output_path.parent_path().empty())
    {
        output_path = std::filesystem::current_path() / output_path;
    }

    // Create directories if they don't exist (this won't do anything if it's
    // just a filename)
    std::filesystem::create_directories(output_path.parent_path());

    std::ofstream output_file(output_path);
    if (!output_file.is_open())
    {
        console::error("Unable to open output file: " + output_path.string());
        return 1;
    }
    output_file << result;
    output_file.close();

    console::success("Output written to: " + output_path.string());
    console::blank();
    console::summary("Summary: " +
                     console::pluralize(stats.files, "file") + ", " +
                     console::pluralize(stats.artboards, "artboard") + ", " +
                     console::pluralize(stats.animations, "animation") + ", " +
                     console::pluralize(stats.stateMachines, "state machine") + ", " +
                     console::pluralize(stats.assets, "asset"));
    console::summary("Done in " + timer.elapsedStr());
    console::blank();

    // Signal failure to callers/CI if any input file failed to process.
    if (stats.errors > 0)
    {
        console::error(
            console::pluralize(stats.errors, "file") +
            " failed to process");
        return 1;
    }

    return 0;
}