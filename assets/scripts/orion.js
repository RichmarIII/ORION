// Variables to hold the MediaRecorder instance and the recorded audio chunks
let voiceRecorder;
let audioChunks = [];
let micHoldTimer;
let micHoldThreshold = 250;

let currentMessageBeingComposed = '';
let currentMessageBeingComposedForSpeech = '';
let currentAnnotations = [];

let currentToolBeingComposedName = '';
let currentToolBeingComposedArguments = '';
let currentToolBeingComposedOutput = '';

let orionSpeakQueue = [];
let shouldProcessOrionSpeakQueue = false;

let audioQueue = [];
let shouldProcessAudioQueue = false;

async function playAudioAsync(audioBlob) {
    return new Promise((resolve, reject) => {
        let audio = document.getElementById('audio');
        audio.src = URL.createObjectURL(audioBlob);
        audio.preload = 'none';
        audio.play();

        // When the audio has finished playing, resolve the promise
        audio.onended = function () {
            resolve();
        };
    });
}

async function stopAudioAsync() {
    return new Promise((resolve, reject) => {
        let audio = document.getElementById('audio');
        orionSpeakQueue.slice(0, orionSpeakQueue.length); // Clear the orion speak queue
        audioQueue.slice(0, audioQueue.length); // Clear the audio queue
        audio.pause();
        audio.currentTime = 0;
        resolve();
    });
}

// Function to start recording
function startVoiceRecording() {
    // Get the microphone access
    navigator.mediaDevices.getUserMedia({audio: true})
        .then(stream => {
            audioChunks = [];

            voiceRecorder = new MediaRecorder(stream);

            voiceRecorder.ondataavailable = function (event) {
                if (event.data) audioChunks.push(event.data);
            };

            voiceRecorder.onstop = function () {
                // Stop the microphone stream
                stream.getTracks().forEach(track => track.stop());

                if (audioChunks.length === 0) {
                    console.error('No audio data recorded');
                    return;
                }

                // Get the MIME type of the audio
                const mimeType = voiceRecorder.mimeType;

                // Combine the audio chunks into a single Blob
                const audioBlob = new Blob(audioChunks, {type: mimeType});

                // Play a beep sound to indicate that recording has ended.
                const audio = document.getElementById('audio');
                audio.src = "/assets/audio/mic_stop.mp3";
                audio.play();

                // Send the audio data to the server
                OrionAPI.transcribeAudioAsync(audioBlob)
                    .then(message => {
                        addMessageToChatAsync(message, []);
                    });
            };

            // Play a beep sound to indicate that recording has started.
            let audio = document.getElementById('audio');
            audio.src = "/assets/audio/mic_start.mp3";
            audio.play();

            voiceRecorder.start();
        }).catch(error => {
        console.error('Error getting microphone access: ', error);
        alert('Please ensure a microphone is connected and permissions are granted.' + error);
    });
}

// Function to stop recording
function stopVoiceRecording() {
    if (voiceRecorder) {
        voiceRecorder.stop();
    }
}

// Event listeners for the mic button
document.getElementById('send-button').addEventListener('touchstart', function (event) {
    event.preventDefault();
    micHoldTimer = setTimeout(startVoiceRecording, micHoldThreshold);
});

document.getElementById('send-button').addEventListener('mousedown', function (event) {
    event.preventDefault();
    micHoldTimer = setTimeout(startVoiceRecording, micHoldThreshold);
});

document.getElementById('send-button').addEventListener('mouseup', function (event) {
    event.preventDefault();
    clearTimeout(micHoldTimer);
    stopVoiceRecording();
    const messageInput = document.getElementById('message-input').value;
    const fileInput = document.getElementById('file-input').files;
    if (messageInput.trim() !== '' || fileInput.length > 0) {
        console.log('fileInput:', fileInput);

        // Create a promise to read the files
        const promise = new Promise((resolve, reject) => {
            const Files = [];
            let filesRead = 0;

            if (fileInput.length === 0) {
                resolve(Files);
            }

            // Read the files and add them to the Files array
            for (let i = 0; i < fileInput.length; i++) {
                let file = fileInput[i]; // Use let to create a block-scoped variable
                let reader = new FileReader();
                reader.onload = function (e) {
                    Files.push({name: file.name, data: e.target.result.split(',')[1]}); // Remove the data URL prefix
                    filesRead++;
                    if (filesRead === fileInput.length) {
                        resolve(Files); // Resolve the promise when all files are read
                    }
                };
                reader.onerror = function (e) {
                    reject(e); // Reject the promise if there's an error reading a file
                };
                reader.readAsDataURL(file);
            }
        });

        // Wait for the files to be read
        promise.then((Files) => {
            // Add the message to the chat area
            addMessageToChatAsync(messageInput, Files);
            document.getElementById('message-input').value = '';
            document.getElementById('file-input').value = '';
        }).catch((error) => {
            console.error('Error reading files:', error);
        });
    }
});

document.getElementById('send-button').addEventListener('touchend', function (event) {
    event.preventDefault();
    clearTimeout(micHoldTimer);
    stopVoiceRecording();
    const messageInput = document.getElementById('message-input').value;
    const fileInput = document.getElementById('file-input').files;
    if (messageInput.trim() !== '' || fileInput.length > 0) {
        console.log('fileInput:', fileInput);

        // Create a promise to read the files
        const promise = new Promise((resolve, reject) => {
            const Files = [];
            let filesRead = 0;

            if (fileInput.length === 0) {
                resolve(Files);
            }

            // Read the files and add them to the Files array
            for (var i = 0; i < fileInput.length; i++) {
                let file = fileInput[i]; // Use let to create a block-scoped variable
                let reader = new FileReader();
                reader.onload = function (e) {
                    Files.push({name: file.name, data: e.target.result.split(',')[1]}); // Remove the data URL prefix
                    filesRead++;
                    if (filesRead === fileInput.length) {
                        resolve(Files); // Resolve the promise when all files are read
                    }
                };
                reader.onerror = function (e) {
                    reject(e); // Reject the promise if there's an error reading a file
                };
                reader.readAsDataURL(file);
            }
        });

        // Wait for the files to be read
        promise.then((Files) => {
            // Add the message to the chat area
            addMessageToChatAsync(messageInput, Files);
            document.getElementById('message-input').value = '';
            document.getElementById('file-input').value = '';
        }).catch((error) => {
            console.error('Error reading files:', error);
        });
    }
});
document.getElementById('send-button').addEventListener('mouseleave', function (event) {
    event.preventDefault();
    clearTimeout(micHoldTimer);
    stopVoiceRecording();
});

// Listen for server-sent events
const orionEventsSource = new EventSource('/orion/events');
orionEventsSource.addEventListener('message.started', function (event) {
    // This event is triggered when Orion starts to compose a message

    currentMessageBeingComposed = '';
    currentAnnotations = [];

    // We want to create a new message for Orion initially and then append the delta to the existing message as it is streamed in real-time

    // Get the chat area
    const chatArea = document.getElementById('chat-area');

    // Create a new message for orion
    const newMessage = createOrionMessage('');

    // Add the new message to the chat area
    chatArea.appendChild(newMessage);

    // Scroll to the bottom of the chat area
    chatArea.scrollTop = chatArea.scrollHeight;
});

async function startProcessingAudioQueueAsync() {
    shouldProcessAudioQueue = true;
    while (true) {
        if (shouldProcessAudioQueue !== false) {
            if (audioQueue.length > 0) {
                const audioBlob = audioQueue.shift();
                await playAudioAsync(audioBlob);
            } else {
                await new Promise(resolve => setTimeout(resolve, 100));
            }
        } else {
            break;
        }
    }
}

// Asynchronously processes the Orion speak queue as they are added in a loop foreach sentence
async function startProcessingOrionSpeakQueueAsync() {
    shouldProcessOrionSpeakQueue = true;
    while (true) {
        if (shouldProcessOrionSpeakQueue !== false) {
            if (orionSpeakQueue.length > 0) {
                const sentence = orionSpeakQueue.shift();

                // Generate the audio for the sentence
                const audioBlob = await OrionAPI.speakAsync(sentence);

                // Add the audio to the audio queue
                audioQueue.push(audioBlob);

            } else {
                await new Promise(resolve => setTimeout(resolve, 100));
            }
        } else {
            break;
        }
    }
}

async function orionSpeakAsync(sentence) {
    // Add the sentence to the queue
    orionSpeakQueue.push(sentence);
}

function renderPreviewImages(messageElement) {
    // for all links in the output, add an image tag as child of the link for preview
    const links = messageElement.querySelectorAll('a');
    links.forEach(link => {

        // Process the link based on the file type
        if (link.href.match(/\.(jpeg|jpg|gif|png|svg)$/) || link.href.includes('data:image')) {
            const img = document.createElement('img');
            img.src = link.href;
            img.alt = link.href;
            img.style.maxWidth = '100%';
            img.style.maxHeight = '100%';
            link.appendChild(img);
        } else if (link.href.match(/\.(pdf)$/) || link.href.includes('data:application/pdf')) {
            const pdf = document.createElement('embed');
            pdf.src = link.href;
            pdf.type = 'application/pdf';
            pdf.width = '100%';
            pdf.height = '100%';
            link.appendChild(pdf);
        } else if (link.href.match(/\.(mp4|webm|ogg)$/) || link.href.includes('data:video')) {
            const video = document.createElement('video');
            video.src = link.href;
            video.controls = true;
            video.width = '100%';
            video.height = '100%';
            video.style.maxWidth = '100%';
            video.style.maxHeight = '100%';
            link.appendChild(video);
        }
        // Render youtube videos
        else if (link.href.match(/youtube.com\/watch\?v=([a-zA-Z0-9_-]+)/)) {
            const videoId = link.href.match(/youtube.com\/watch\?v=([a-zA-Z0-9_-]+)/)[1];
            const iframe = document.createElement('iframe');
            iframe.src = `https://www.youtube.com/embed/${videoId}`;
            iframe.width = '100%';
            iframe.height = '100%';
            iframe.allow = 'accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture';
            iframe.allowFullscreen = true;
            link.appendChild(iframe);
        }
    });
}

orionEventsSource.addEventListener('message.delta', async function (event) {
    // This event is triggered when Orion sends a delta of the message being composed

    // We want to append the new message to the existing message created by 'message.started'
    // This is because we are streaming the message in real-time. We don't want to create a new message for each delta.

    currentMessageBeingComposed += JSON.parse(event.data).message;

    currentMessageBeingComposedForSpeech += JSON.parse(event.data).message;

    // Grab the first sentence of the message (up to the first period, exclamation point, or question mark)
    const sentence = currentMessageBeingComposedForSpeech.split(/\.\!\?/)[0];

    if (sentence.endsWith('.') || sentence.endsWith('!') || sentence.endsWith('?') && sentence.length > 15) {
        // Remove the first sentence from the message
        currentMessageBeingComposedForSpeech = currentMessageBeingComposedForSpeech.substring(sentence.length);

        await orionSpeakAsync(sentence);
    }

    // Render the markdown into html
    await OrionAPI.renderMarkdownAsync(currentMessageBeingComposed)
        .then(markdown => {
            // Get the chat area
            const chatArea = document.getElementById('chat-area');

            // Get the last message in the chat area
            const lastElement = chatArea.querySelector('.orion-message-container:last-child')

            if (lastElement) {
                // Append the new message to the existing message
                const lastMessageElement = lastElement.querySelector('.message');

                if (lastMessageElement) {
                    // Update the message with the new message
                    lastMessageElement.innerHTML = markdown;

                    // scroll to the bottom of the chat area
                    chatArea.scrollTop = chatArea.scrollHeight;
                }
            }

        });
});

orionEventsSource.addEventListener('message.in_progress', function (event) {
    // This event is triggered when Orion is in the process of composing a message

    // We want to update the existing message container with the in-progress message

    // Get the chat area
    const chatArea = document.getElementById('chat-area');

    // Get the last message in the chat area
    const lastMessage = chatArea.lastChild;

    // Create a new div element for the in-progress message
    const inProgressDiv = document.createElement('div');
    inProgressDiv.className = 'message-status';
    inProgressDiv.textContent = 'Orion is composing a message...';

    // insert the in-progress message before the message div
    lastMessage.insertBefore(inProgressDiv, lastMessage.querySelector('.message'));

});

orionEventsSource.addEventListener('message.completed', async function (event) {
    // This event is triggered when Orion completes composing a message

    // Play the final sentence if it exists
    if (currentMessageBeingComposedForSpeech.length > 0) {
        await orionSpeakAsync(currentMessageBeingComposedForSpeech);
    }

    currentMessageBeingComposedForSpeech = '';

    // We want to append the final message to the existing message created by 'message.started' and 'message.delta'

    // Get the chat area
    const chatArea = document.getElementById('chat-area');

    // Get the last message of class 'orion-message-container'
    const lastMessage = chatArea.querySelector('.orion-message-container:last-child');

    // convert the message to markdown
    let orionText = currentMessageBeingComposed;

    currentMessageBeingComposed = '';

    // Loop through the annotations and replace the placeholders in the message with the actual values
    currentAnnotations.forEach(annotation => {
        orionText = orionText.replace(annotation.text_to_replace, annotation.url);
    });

    // log annotations
    console.log(currentAnnotations);

    // Clear the annotations
    currentAnnotations = [];

    // If the message is empty, do not proceed with rendering markdown
    if (orionText.trim() === '') {
        // Remove the in-progress message
        const messageStatus = lastMessage.querySelector('.message-status')
        if (messageStatus) {
            messageStatus.remove();
        }
        return;
    }

    // Render markdown into html
    await OrionAPI.renderMarkdownAsync(orionText)
        .then(markdown => {
            // Remove the in-progress message
            lastMessage.querySelector('.message-status').remove();

            // Update the message with the final message
            lastMessage.querySelector('.message').innerHTML = markdown;

            // Render the preview images
            renderPreviewImages(lastMessage);

            // Process the new message for code blocks and add copy buttons
            processCodeBlocks(lastMessage);

            // Scroll to the bottom of the chat area
            chatArea.scrollTop = chatArea.scrollHeight;
        });
});

orionEventsSource.addEventListener('message.annotation.created', function (event) {
    // This event is triggered when Orion creates an annotation for the message.
    // Should be saved for later use.  When message is completed, the annotations should be added to the message

    currentAnnotations.push(JSON.parse(event.data));
});

orionEventsSource.addEventListener('upload.file.requested', function (event) {
    // This event is triggered when Orion requests a file upload from the user
    const jdata = JSON.parse(event.data);
    const filePath = jdata['file_path'];

    console.log('Requested upload file:', filePath);
});

orionEventsSource.addEventListener('tool.started', function (event) {
    // This event is triggered when a tool starts
    const jdata = JSON.parse(event.data);

    currentToolBeingComposedName = '';
    currentToolBeingComposedArguments = '';
    currentToolBeingComposedOutput = '';

    // Create a tool message and add it to the chat area
    const newMessage = createToolMessage("");
    const chatArea = document.getElementById('chat-area');
    chatArea.appendChild(newMessage);
    chatArea.scrollTop = chatArea.scrollHeight;
});

orionEventsSource.addEventListener('tool.delta', function (event) {
    // This event is triggered when a tool sends a delta
    const jdata = JSON.parse(event.data);

    if (jdata.name !== '') {
        currentToolBeingComposedName = jdata.name;
    }

    if (jdata.args !== '') {
        currentToolBeingComposedArguments += jdata.args;
    }

    // Get the chat area
    const chatArea = document.getElementById('chat-area');

    // Get the last message in the chat area
    const lastMessage = chatArea.lastElementChild;

    // Append the new message to the existing message
    const lastMessageElement = lastMessage.querySelector('.message');

    const toolMessage = {}
    toolMessage.name = currentToolBeingComposedName;
    toolMessage.arguments = currentToolBeingComposedArguments;
    toolMessage.output = currentToolBeingComposedOutput;

    lastMessageElement.innerHTML = JSON.stringify(toolMessage, null, 2);

    // Scroll to the bottom of the chat area
    chatArea.scrollTop = chatArea.scrollHeight;
});

orionEventsSource.addEventListener('tool.completed', function (event) {
    // This event is triggered when a tool completes
    const jdata = JSON.parse(event.data);

    // Get the chat area
    const chatArea = document.getElementById('chat-area');

    // Get the last message in the chat area that contains the class 'tool-message-container'
    const lastMessage = chatArea.lastElementChild;

    // Append the new message to the existing message
    const lastMessageElement = lastMessage.querySelector('.message');

    const toolMessage = {}
    toolMessage.name = jdata.name;

    // Try to parse the arguments as JSON
    try {
        toolMessage.arguments = JSON.parse(jdata.args);
    } catch (error) {
        toolMessage.arguments = jdata.args;
    }

    // Try to parse the output as JSON
    try {
        toolMessage.output = JSON.parse(jdata.output);
    } catch (error) {
        toolMessage.output = jdata.output;
    }

    let toolString = '';
    if (toolMessage.name === 'code_interpreter') {
        toolString = toolMessage.arguments;

        // wrap the tool message in a python code block
        toolString = '```python\n' + toolString + '\n```';
    } else {
        toolString = JSON.stringify(toolMessage, null, 2);

        // wrap the tool message in a json code block
        toolString = '```json\n' + toolString + '\n```';
    }

    console.log('Tool message:', toolString);

    OrionAPI.renderMarkdownAsync(toolString)
        .then(markdown => {
            // Update the message with the final message
            lastMessageElement.innerHTML = markdown;

            // Render the preview images
            renderPreviewImages(lastMessageElement);

            // Process the new message for code blocks and add copy buttons
            processCodeBlocks(lastMessage);

            // Scroll to the bottom of the chat area
            chatArea.scrollTop = chatArea.scrollHeight;
        });
});

// Many browsers do not support playing audio files without user interaction.
// As a workaround, when the user touches the screen, we will play a silent audio file.
// This will allow us to play audio files later without user interaction.
document.addEventListener('touchstart', function () {
    // Get the audio element
    let audio = document.getElementById('audio')

    // This is a tiny MP3 file that is silent and extremely short
    audio.src = "data:audio/mpeg;base64,SUQzBAAAAAABEVRYWFgAAAAtAAADY29tbWVudABCaWdTb3VuZEJhbmsuY29tIC8gTGFTb25vdGhlcXVlLm9yZwBURU5DAAAAHQAAA1N3aXRjaCBQbHVzIMKpIE5DSCBTb2Z0d2FyZQBUSVQyAAAABgAAAzIyMzUAVFNTRQAAAA8AAANMYXZmNTcuODMuMTAwAAAAAAAAAAAAAAD/80DEAAAAA0gAAAAATEFNRTMuMTAwVVVVVVVVVVVVVUxBTUUzLjEwMFVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVf/zQsRbAAADSAAAAABVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVf/zQMSkAAADSAAAAABVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV";
    audio.play();

    // Remove the event listener so that the audio is only played once
    document.removeEventListener('touchstart', arguments.callee);
});

// Add event listener to text input to send message on pressing enter
document.getElementById('message-input').addEventListener('keypress', function (e) {
    if (e.key === 'Enter') {
        const messageInput = document.getElementById('message-input').value;
        const fileInput = document.getElementById('file-input').files;
        if (messageInput.trim() !== '' || fileInput.length > 0) {
            console.log('fileInput:', fileInput);

            // Create a promise to read the files
            const promise = new Promise((resolve, reject) => {
                const Files = [];
                let filesRead = 0;

                if (fileInput.length === 0) {
                    resolve(Files);
                }

                // Read the files and add them to the Files array
                for (let i = 0; i < fileInput.length; i++) {
                    let file = fileInput[i]; // Use let to create a block-scoped variable
                    let reader = new FileReader();
                    reader.onload = function (e) {
                        Files.push({name: file.name, data: e.target.result.split(',')[1]}); // Remove the data URL prefix
                        filesRead++;
                        if (filesRead === fileInput.length) {
                            resolve(Files); // Resolve the promise when all files are read
                        }
                    };
                    reader.onerror = function (e) {
                        reject(e); // Reject the promise if there's an error reading a file
                    };
                    reader.readAsDataURL(file);
                }
            });

            // Wait for the files to be read
            promise.then((Files) => {
                // Add the message to the chat area
                addMessageToChatAsync(messageInput, Files).then(() => {
                    document.getElementById('message-input').value = '';
                    document.getElementById('file-input').value = '';
                });
            }).catch((error) => {
                console.error('Error reading files:', error);
            });
        }
    }
});

document.getElementById('new-chat-button').addEventListener('click', function () {
    // Implement the logic to handle new chat creation here
    alert('New chat functionality to be implemented');
});

document.addEventListener('DOMContentLoaded', async function () {

    // Start processing the Orion speak queue
    startProcessingOrionSpeakQueueAsync();

    // Start processing the audio queue
    startProcessingAudioQueueAsync();

    // Get the user id from local storage
    const userID = localStorage.getItem('user_id');
    if (userID === null) {
        console.log('User not found in local storage');

        // Redirect to the login page
        window.location.href = '/assets/html/login.html';
    }

    const plugins = await OrionAPI.getAvailablePluginsAsync();

    // Log the plugins
    console.log('Plugins available:', plugins);

    // Generate the modified plugins
    const modifiedPlugins = plugins.map(plugin => {
        return {
            name: plugin.name,
            enabled: true
        };
    });

    const plugins_results = await OrionAPI.modifyPluginsAsync(modifiedPlugins);

    console.log('Plugins enabled:', plugins_results);

    // Get the chat area
    const chatArea = document.getElementById('chat-area');

    // Get the chat history from the server
    OrionAPI.getChatHistoryAsync()
        .then(messages => {
            let newMessage;
            const chatArea = document.getElementById('chat-area');

            // Loop through the messages and add them to the chat area
            for (let i = 0; i < messages.length; i++) {
                if (messages[i].role === 'user') {
                    // Create a new message for the user
                    newMessage = createUserMessage(messages[i].message);
                    // Add the new message to the chat area
                    chatArea.appendChild(newMessage);
                } else {
                    // Create a new message for orion
                    newMessage = createOrionMessage(messages[i].message);
                    // Add the new message to the chat area
                    chatArea.appendChild(newMessage);
                }
            }

            // Process the new messages for code blocks and add copy buttons
            const newMessages = chatArea.querySelectorAll('.orion-message-container');
            newMessages.forEach(newMessage => {
                renderPreviewImages(newMessage);
                processCodeBlocks(newMessage);
            });

            // Scroll to the bottom of the chat area
            chatArea.scrollTop = chatArea.scrollHeight;

        });
});

// Function to process when the user leaves the page
window.onbeforeunload = function () {
    // Stop processing the Orion speak queue
    shouldProcessOrionSpeakQueue = false;

    // Stop processing the audio queue
    shouldProcessAudioQueue = false;
}

/**
 * Processes the code blocks within a message Element and adds copy buttons to each code block and highlights the code.
 * This modifies the Element in place and therefore does not represent the original texts.  It should no longer
 * be used as if it were the original text after this function is called. (it is for display only).
 * Original texts can be retrieved from the server if needed.
 *
 * @param {HTMLDivElement} messageElement The Element to process. This is a div element containing the message
 * (.orion-message-container, .user-message-container).
 */
function processCodeBlocks(messageElement) {
    const codeBlocks = messageElement.querySelectorAll('pre code');
    if (codeBlocks.length > 0) {
        codeBlocks.forEach(codeBlock => {
            // Add copy button to each code block
            const copyButton = document.createElement('button');
            copyButton.textContent = 'Copy';
            copyButton.className = 'copy-button';
            copyButton.addEventListener('click', function () {
                navigator.clipboard.writeText(codeBlock.textContent).then(function () {
                    console.log('Code copied to clipboard');
                }, function (err) {
                    console.error('Failed to copy code to clipboard', err);
                });
            });

            // Add the copy button to the code block
            codeBlock.parentNode.insertBefore(copyButton, codeBlock);
        });
    }

    // Highlight all code elements within the newMessage
    const codeElements = messageElement.querySelectorAll('code');
    if (codeElements.length > 0) {
        codeElements.forEach(function (codeElement) {
            hljs.highlightElement(codeElement);
        });
    } else {
        console.warn('No <code> elements found within the new message for highlighting.');
    }
}

/**
 * Creates a new message for Orion (but does not add it to the chat area).
 * This is used internally by the addMessageToChatAsync function.
 * @param {string} message The message to display.
 * @returns {HTMLDivElement} The message container.
 */
function createOrionMessage(message) {
    // Create a new div element for the message-container
    const messageContainer = document.createElement('div');
    messageContainer.className = 'orion-message-container';

    // Create a new div element for the image of the speaker
    const image = document.createElement('img');
    image.src = '/assets/images/orion.svg';
    image.alt = 'Orion';
    image.className = 'image';

    // Create a new div element for the message
    const messageDiv = document.createElement('div');
    messageDiv.className = 'message';
    messageDiv.innerHTML = message;

    // Add the image and message to the message-container
    messageContainer.appendChild(image);
    messageContainer.appendChild(messageDiv);

    return messageContainer;
}

/**
 * Creates a new message for a tool (but does not add it to the chat area).
 * @param {string} message The message to display.
 * @returns {HTMLDivElement} The message container.
 */
function createToolMessage(message) {

    // Create a new div element for the collapsible container
    const collapsibleContainer = document.createElement('div');
    collapsibleContainer.className = 'tool-collapsible-container';

    // Create a new div element for the collapsible button
    const collapsibleButton = document.createElement('button');
    collapsibleButton.className = 'tool-collapsible-button';
    collapsibleButton.textContent = 'Tool Output';
    collapsibleButton.addEventListener('click', function () {
        const content = collapsibleContainer.querySelector('.tool-message-container');
        // Toggle the content visibility by adding or removing the 'hidden' class
        content.classList.toggle('hidden');
    });

    // Create a new div element for the collapsible content

    // Create a new div element for the message-container
    const messageContainer = document.createElement('div');
    messageContainer.className = 'tool-message-container';
    messageContainer.classList.add('hidden');

    // Create a new div element for the image of the speaker
    const image = document.createElement('img');
    image.src = '/assets/images/orion.svg';
    image.alt = 'Orion';
    image.className = 'image';

    // Create a new div element for the message
    const messageDiv = document.createElement('div');
    messageDiv.className = 'message';
    messageDiv.innerHTML = message;

    // Add the image and message to the message-container
    messageContainer.appendChild(image);
    messageContainer.appendChild(messageDiv);

    // Add the collapsible button and message to the collapsible container
    collapsibleContainer.appendChild(collapsibleButton);
    collapsibleContainer.appendChild(messageContainer);

    return collapsibleContainer;
}

/**
 * Creates a new message for the user (but does not add it to the chat area).
 * This is used internally by the addMessageToChatAsync function.
 * @param {string} message The message to display.
 * @returns {HTMLDivElement} The message container.
 */
function createUserMessage(message) {
    // Create a new div element for the message-container
    const messageContainer = document.createElement('div');
    messageContainer.className = 'user-message-container';

    // Create a new div element for the image of the speaker
    const image = document.createElement('img');
    image.src = '/assets/images/user.svg';
    image.alt = 'User';
    image.className = 'image';

    // Create a new div element for the message
    const messageDiv = document.createElement('div');
    messageDiv.className = 'message';
    messageDiv.innerHTML = message;

    // Add the image and message to the message-container
    messageContainer.appendChild(image);
    messageContainer.appendChild(messageDiv);

    return messageContainer;
}

/**
 * Adds a message to the chat area and sends the message to Orion.
 * @param {string} message The message to add to the chat area and send to Orion.
 * @param {FileList} files The files to send with the message.
 */
async function addMessageToChatAsync(message, files) {
    if (message.trim() !== '') {

        // Stop any audio that is currently playing
        await stopAudioAsync();

        // Remove the in-progress message if it exists
        const chatArea = document.getElementById('chat-area');
        const lastMessage = chatArea.querySelector('.orion-message-container:last-child');
        if (lastMessage) {
            const messageStatus = lastMessage.querySelector('.message-status');
            if (messageStatus) {
                messageStatus.remove();
            }
        }

        // Render the message as markdown and add it to the chat area
        const renderMarkdownPromise = OrionAPI.renderMarkdownAsync(message)
            .then(markdown => {

                // Create a new message for the user
                const newMessage = createUserMessage(markdown);

                const chatArea = document.getElementById('chat-area');

                // Render the preview images
                renderPreviewImages(newMessage);

                // Add the new message to the chat area
                chatArea.appendChild(newMessage);

                // Scroll to the bottom of the chat area
                chatArea.scrollTop = chatArea.scrollHeight;

            }).catch(error => {
                console.error('Error:', error);
                alert('Failed to convert message to markdown: ' + error);
            });

        // Send the message to Orion
        await OrionAPI.sendMessageAsync(message, files);

        // Wait for the message to be rendered as markdown
        await renderMarkdownPromise;
    }
}