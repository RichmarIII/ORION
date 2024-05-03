#include "TakeScreenshotFunctionTool.hpp"
#include "Orion.hpp"

using namespace ORION;

using FunctionResults = FunctionTool::Statics::FunctionResults;

std::string TakeScreenshotFunctionTool::Execute(Orion& Orion, const web::json::value& Parameters)
{
    // Cross-platform screenshot code

    auto Result                                 = web::json::value::object();
    Result[FunctionResults::NAME_RESULT.data()] = web::json::value::string("not implemented");
    return Result.serialize();
}