/**
 * The Orion API provides a simple way to interact with the Orion server.
 * The API is designed to be used in a web browser and provides methods to interact with the REST API of the Orion server.
 * The API is asynchronous and returns Promises that can be used to handle the results of the requests.
 * The API is designed to be used with the Orion server and requires the server to be running in order to work
 */
class OrionAPI {

    /**
     * Returns the available audio formats for the text-to-speech service.
     * @returns {{MP3: string, AAC: string, FLAC: string, OPUS: string, PCM: string, WAV: string, DEFAULT: string}}
     */
    static get AudioFormat() {
        return {
            MP3: 'mp3', WAV: 'wav', FLAC: 'flac', AAC: 'aac', OPUS: 'opus', PCM: 'pcm', DEFAULT: 'mp3'
        };
    }

    /**
     * Instructs Orion to speak the given text by using the text-to-speech service and returns the audio file.
     * @param {string} text The text to be spoken.
     * @param {string} audioFormat The audio format of the returned audio file.
     * @returns {Promise<Blob>} The audio file.
     */
    static async speakAsync(text, audioFormat = OrionAPI.AudioFormat.DEFAULT) {
        return fetch('/orion/speak?format=' + audioFormat, {
            method: 'POST', headers: {
                'Content-Type': 'application/json', 'X-User-Id': localStorage.getItem('user_id'),
            }, body: JSON.stringify({message: text}),
        }).then(response => response.blob());
    }

    /**
     * Sends a chat message to orion.
     * @param message {string} The message to send. This can be a simple text message or a markdown message.
     * @param files {FileList} A list of files to send with the message. max 10 files per message.
     * @param shouldProcessMarkdown {boolean} If true, the message will be processed as markdown (markdown will be rendered and html returned).
     * @returns {Promise<Response>} The response from the server. This can be used to check if the message was sent successfully.
     * The server will send sse events back to the client with orion's response as it's streamed in via the 'message.delta' event.
     */
    static async sendMessageAsync(message, files = [], shouldProcessMarkdown = true) {

        // Empty messages are not allowed
        if (!message) {
            console.warn('The message is empty. the message "empty message" will be sent instead.');
            message = 'empty message';
        }

        // Create a body that contains the message and the files
        const body = {
            message: message, files: [...files] // Convert the FileList to an array
        };

        // Limit the number of files to 10 (max allowed by the server)
        if (body.files.length > 10) {
            console.warn('The number of files exceeds the maximum allowed by the server. Only the first 10 files will be sent.');

            body.files = body.files.slice(0, 10);
        }

        // Send the message to the server
        return fetch('/orion/send_message?markdown=' + shouldProcessMarkdown, {
            method: 'POST', headers: {
                'X-User-Id': localStorage.getItem('user_id'), 'Content-Type': 'application/json'
            }, body: JSON.stringify(body)
        });
    }

    /**
     * Renders the given markdown text and returns the rendered HTML.
     * @param markdown {string} The markdown text to render.
     * @returns {Promise<string>} The rendered HTML.
     */
    static async renderMarkdownAsync(markdown) {
        return fetch('/markdown', {
            method: 'POST', headers: {
                'Content-Type': 'application/json'
            }, body: JSON.stringify({message: markdown})
        }).then(response => response.json())
            .then(data => data.message);
    }

    /**
     * Returns the chat history from the server.
     * @param processMarkdown {boolean} If true, markdown will be rendered and the HTML will be returned.
     * @returns {Promise<Array>} The chat history. Each message is an object with the following properties:
     * - role: Who sent the message (user or orion).
     * - message: The message text.
     */
    static async getChatHistoryAsync(processMarkdown = true) {
        // Fetch the chat messages from the server
        return fetch('/orion/chat_history?markdown=' + processMarkdown, {
            method: 'GET', headers: {
                'X-User-Id': localStorage.getItem('user_id')
            }
        }).then(response => response.json())
            .then(data => data['messages']);
    }

    /**
     * Transcribes the given audio data and returns the transcribed text.
     * @param audioBlob {Blob} The audio data to transcribe.
     * @param processMarkdown {boolean} If true, the transcribed text will be rendered as markdown and the HTML will be returned.
     * @returns {Promise<string>} The transcribed text.
     */
    static async transcribeAudioAsync(audioBlob, processMarkdown = true) {

        // Send the audio file to the server
        return fetch('/orion/transcribe?markdown=' + processMarkdown, {
            method: 'POST', body: audioBlob, headers: {
                'Content-Type': audioBlob.type,
            },
        }).then(response => response.json())
            .then(data => data['message']);
    }
}