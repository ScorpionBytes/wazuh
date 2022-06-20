#include "stageBuilderMap.hpp"

#include <algorithm>
#include <any>

#include "baseTypes.hpp"
#include "builder/expression.hpp"
#include "json.hpp"
#include "builder/registry.hpp"

namespace builder::internals::builders
{

Expression stageMapBuilder(std::any definition)
{
    json::Json jsonDefinition;

    try
    {
        jsonDefinition = std::any_cast<json::Json>(definition);
    }
    catch (std::exception& e)
    {
        throw std::runtime_error(
            "[builders::stageMapBuilder(json)] Received unexpected argument type");
    }

    if (!jsonDefinition.isObject())
    {
        throw std::runtime_error(
            fmt::format("[builders::stageMapBuilder(json)] Invalid json definition "
                        "type: expected [object] but got [{}]",
                        jsonDefinition.typeName()));
    }

    auto mappings = jsonDefinition.getObject();
    std::vector<Expression> mappingExpressions;
    std::transform(mappings.begin(),
                   mappings.end(),
                   std::back_inserter(mappingExpressions),
                   [](auto tuple)
                   { return Registry::getBuilder("operation.map")(tuple); });

    auto expression = Chain::create("stage.map", mappingExpressions);
    return expression;
}

} // namespace builder::internals::builders