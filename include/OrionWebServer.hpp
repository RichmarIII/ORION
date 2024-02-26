#pragma once

#include <cpprest/http_listener.h>
#include <vector>
#include <memory>
#include "Orion.hpp"

namespace ORION
{
    class Orion;

    /// @brief  A web server that can be used to host the Orion web interface and provide a REST API for Orion.
    /// @note   All requests must include the X-Orion-Id header with the id of the Orion instance you want to interact with.
    class OrionWebServer
    {
    public:
        /// @brief  Destructor (virtual for inheritance)
        virtual ~OrionWebServer() = default;

        /// @brief  Start the web server
        /// @param  port The port to listen on
        void Start(int port);

        /// @brief  Stop the web server
        void Stop();

        /// @brief  Wait for the web server to stop
        void Wait();

    protected:
        /// @brief  Dispatches a request to the appropriate handler based on the request method and path
        /// @param  request The HTTP request
        void HandleRequest(web::http::http_request request);

        /// @brief  The /send_message endpoint is used to send a message to Orion. optionally converting it to markdown via the ?markdown=true query
        /// parameter
        /// @param  request The HTTP request
        /// @example curl -X POST -d "Hello, Orion!" http://localhost:8080/send_message
        /// @example Response: "Hello, user!"
        /// @example curl -X POST -d "Hello, Orion!" http://localhost:8080/send_message?markdown=true
        /// @example Response: "<p>Hello, user!</p>"
        void HandleSendMessageEndpoint(web::http::http_request request);

        /// @brief  The / endpoint is used to serve the Orion web interface
        /// @param  request The HTTP request
        /// @example curl -X GET http://localhost:8080/
        /// @example Response: The contents of the index.html file
        void HandleRootEndpoint(web::http::http_request request);

        /// @brief  The /<file> endpoint is used to serve static files.
        ///         This is a catch-all endpoint that will serve any file in the static directory.
        /// @param  request The HTTP request
        /// @example curl -X GET http://localhost:8080/image.png
        /// @example Response: The contents of the image.png file served from the static directory
        void HandleStaticFileEndpoint(web::http::http_request request);

        /// @brief  The /markdown endpoint is used to convert a message to markdown
        /// @param  request The HTTP request
        /// @example curl -X POST -d "Hello, Orion!" http://localhost:8080/markdown
        /// @example Response: "<p>Hello, Orion!"</p>
        void HandleMarkdownEndpoint(web::http::http_request request);

        /// @brief  The /chat_history endpoint is used to retrieve the chat history.
        /// Optionally converting it to markdown via the ?markdown=true query parameter
        /// @param  request The HTTP request
        /// @example curl -X GET http://localhost:8080/chat_history
        /// @example Response: [{ "role": "user", "message": "Hello, Orion!" }, { "role": "orion", "message": "Hello, user!" }]
        /// @example curl -X GET http://localhost:8080/chat_history?markdown=true
        /// @example Response: [{ "role": "user", "message": "<p>Hello, Orion!</p>" }, { "role": "orion", "message": "<p>Hello, user!</p>" }]
        void HandleChatHistoryEndpoint(web::http::http_request request);

        /// @brief  The /speak endpoint is used to make Orion speak a message.
        /// Supported audio formats: mp3, opus, aac, flac, wav, and pcm
        /// Audio is segmented into multiple files if the message is too long. Return a json array of audio files
        /// that can be played using the play_audio endpoint.
        /// @param  request The HTTP request
        /// @example curl -X POST -d "Hello, Orion!" http://localhost:8080/speak
        /// @example Response: [{ "file": "audio1.mp3" }, { "file": "audio2.mp3" }]
        /// @example curl -X POST -d "Hello, Orion!" http://localhost:8080/speak?format=opus
        /// @example Response: [{ "file": "audio1.opus" }, { "file": "audio2.opus" }]
        /// @example curl -X POST -d "Hello, Orion!" http://localhost:8080/speak?format=wav
        /// @example Response: [{ "file": "audio1.wav" }, { "file": "audio2.wav" }]
        /// @note   The audio files are stored in the audio directory
        void HandleSpeakEndpoint(web::http::http_request request);

        /// @brief  The /play_audio endpoint is used to play an audio file
        /// @param  request The HTTP request
        /// @example curl -X GET http://localhost:8080/play_audio?file=audio1.mp3
        /// @example Response: The contents of the audio1.mp3 file served from the audio directory
        void HandlePlayAudioEndpoint(web::http::http_request request);

        /// @brief  The /create_orion endpoint is used to create an Orion instance.
        /// The Orion instance is returned as a JSON object with an id property. This id must be used in the X-Orion-Id header for all requests.
        /// If you want to retrieve an existing Orion instance, you can send the id as a query parameter.
        /// @param  request The HTTP request
        /// @example curl -X POST http://localhost:8080/create_orion
        /// @example Response: { "id": "1234" }
        /// @example curl -X POST http://localhost:8080/create_orion?id=1234
        /// @example Response: { "id": "1234" }
        /// @note   The Orion instance id should be stored and used in the X-Orion-Id header for all requests.
        void HandleCreateOrionEndpoint(web::http::http_request request);

        /// @brief  The orion instances that were created this session
        std::vector<std::unique_ptr<Orion>> m_OrionInstances;

        /// @brief  The listener for the web server
        web::http::experimental::listener::http_listener m_Listener;

        /// @brief  Whether the web server is running
        std::atomic<bool> m_bRunning = false;

        /// @brief  The condition variable for the web server. Signaled when the web server is stopped
        std::condition_variable m_ConditionVariable;

        /// @brief  The mutex for the web server
        std::mutex m_Mutex;
    };
} // namespace ORION