#pragma once

#include "tools/FunctionTool.hpp"

namespace ORION
{
    /// @brief  A tool that can recall knowledge
    class RecallKnowledgeFunctionTool final : public FunctionTool
    {
    public:
        struct Statics
        {
            /// @brief A function that recalls knowledge
            constexpr static auto RECALL_KNOWLEDGE = R"(
            {
                "description" : "Recalls knowledge from the assistant's memory.",
                "name" : "recall_knowledge",
                "parameters" : {
                    "type" : "object",
                    "properties" : {
                        "knowledge_types" : {
                            "type" : "array",
                            "items" : {
                                "type" : "string",
                                "enum" : [ "user_personal_info", "user_interests", "family_personal_info", "family_interests", "user_preferences", "unknown" ]
                            },
                            "description" : "The type of knowledge to recall. Can search multiple databases"
                        },
                        "knowledge_subject_and_tags" : {
                            "type" : "array",
                            "items" : {
                                "type" : "string"
                            },
                            "description" : "MUST contain a list of at LEAST 10 generated TAGS that describes the content. For example, if the knowledge is about a person in a red shirt and black pants walking down the road, the tags could be 'person', 'red shirt', 'black pants', 'walking', 'road', 'outside', 'person walking', 'clothing', 'color', 'activity'."
                        }
                    },
                    "required" : [ "knowledge_types", "knowledge_subject_and_tags"]
                }
            })";
        };

        inline RecallKnowledgeFunctionTool() : FunctionTool(Statics::RECALL_KNOWLEDGE)
        {
        }

        virtual std::string Execute(class Orion& Orion, const web::json::value& Parameters) override;
    };
} // namespace ORION
