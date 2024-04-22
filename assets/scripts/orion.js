//On holding the microphone button, start recording

// Variables to hold the MediaRecorder instance and the recorded audio chunks
let voiceRecorder;
let audioChunks = [];
let micHoldTimer;
let micHoldThreshold = 250;

let currentMessageBeingComposed = '';
let currentMessageBeingComposedForSpeech = '';
let currentAnnotations = [];

let orionSpeakQueue = [];
let isProcessingOrionSpeak = false;

let audioQueue = [];
let isPlayingAudio = false;

function playAudio(audioBlob) {
    // if the audio is playing, add it to the queue
    if (isPlayingAudio) {
        audioQueue.push(audioBlob);
        return;
    }

    isPlayingAudio = true;

    let audio = document.getElementById('audio');
    audio.src = URL.createObjectURL(audioBlob);
    audio.play();

    audio.onended = function () {
        isPlayingAudio = false;
        if (audioQueue.length > 0) {
            playAudio(audioQueue.shift());
        }
    };
}

// Function to start recording
function startVoiceRecording() {
    // Get the microphone access
    navigator.mediaDevices.getUserMedia({audio: true})
        .then(stream => {
            audioChunks = [];

            voiceRecorder = new MediaRecorder(stream);

            voiceRecorder.ondataavailable = function (event) {
                if (event.data)
                    audioChunks.push(event.data);
            };

            voiceRecorder.onstop = function () {
                // Stop the microphone stream
                stream.getTracks().forEach(track => track.stop());

                if (audioChunks.length === 0) {
                    console.error('No audio data recorded');
                    return;
                }

                // Get the MIME type of the audio
                var mimeType = voiceRecorder.mimeType;

                // Combine the audio chunks into a single Blob
                var audioBlob = new Blob(audioChunks, {type: mimeType});

                // Play a beep sound to indicate that recording has ended.
                let audio = document.getElementById('audio');
                audio.src = "mic_stop.mp3";
                audio.play();

                // Send the audio data to the server
                fetch('/stt', {
                    method: 'POST',
                    body: audioBlob,
                    headers: {
                        'Content-Type': mimeType,
                    },
                })
                    .then(response => response.json())
                    .then(data => {
                        addMessageToChat(data.message, []);
                    })
                    .catch(error => {
                        console.error('Error sending audio data:', error);
                    });
            };

            // Play a beep sound to indicate that recording has started.
            let audio = document.getElementById('audio');
            audio.src = "mic_start.mp3";
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
            addMessageToChat(messageInput, Files);
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
    var messageInput = document.getElementById('message-input').value;
    var fileInput = document.getElementById('file-input').files;
    if (messageInput.trim() !== '' || fileInput.length > 0) {
        console.log('fileInput:', fileInput);

        // Create a promise to read the files
        var promise = new Promise((resolve, reject) => {
            var Files = [];
            var filesRead = 0;

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
            addMessageToChat(messageInput, Files);
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
const orionEventsSource = new EventSource('/orion_events');
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

async function orionSpeakAsync(sentence) {
    if (!isProcessingOrionSpeak) {
        isProcessingOrionSpeak = true;

        // Send the sentence to the server for text-to-speech conversion
        fetch('/orion/speak', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
                'X-User-Id': localStorage.getItem('user_id')
            },
            body: JSON.stringify({message: sentence})
        }).then(response => response.blob())
            .then(blob => {
                isProcessingOrionSpeak = false;
                playAudio(blob);

                if (orionSpeakQueue.length > 0) {
                    orionSpeakAsync(orionSpeakQueue.shift());
                }
            }).catch(error => {
            console.error('Error:', error);
            alert('Failed to convert message to speech: ' + error);
        });
    } else {
        orionSpeakQueue.push(sentence);
    }
}

orionEventsSource.addEventListener('message.delta', function (event) {
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

        orionSpeakAsync(sentence);
    }

    // Fetch markdown from the server
    fetch('/markdown',
        {
            method: 'POST',
            headers:
                {
                    'Content-Type': 'application/json'
                },
            body: JSON.stringify({message: currentMessageBeingComposed})
        })
        .then(response => response.json())
        .then(jmarkdown => {
            // Get the chat area
            const chatArea = document.getElementById('chat-area');

            // Get the last message in the chat area
            const newMessage = chatArea.lastChild;

            // Append the new message to the existing message
            const messageDiv = newMessage.querySelector('.message');

            messageDiv.innerHTML = jmarkdown.message;

            // scroll to the bottom of the chat area
            chatArea.scrollTop = chatArea.scrollHeight;
        }).catch(error => {
        console.error('Error:', error);
        alert('Failed to convert message to markdown: ' + error);
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

orionEventsSource.addEventListener('message.completed', function (event) {
    // This event is triggered when Orion completes composing a message

    // Play the final sentence if it exists
    if (currentMessageBeingComposedForSpeech.length > 0) {
        orionSpeakAsync(currentMessageBeingComposedForSpeech);
    }

    currentMessageBeingComposedForSpeech = '';

    // We want to append the final message to the existing message created by 'message.started' and 'message.delta'

    // Get the chat area
    const chatArea = document.getElementById('chat-area');

    // Get the last message in the chat area
    const lastMessage = chatArea.querySelector('.orion-message-container:last-child')

    // convert the message to markdown
    let orionText = currentMessageBeingComposed;

    currentMessageBeingComposed = '';

    // Loop through the annotations and replace the placeholders in the message with the actual values
    currentAnnotations.forEach(annotation => {
        orionText = orionText.replace(annotation.text_to_replace, annotation.file_name);
    });

    // log annotations
    console.log(currentAnnotations);

    // Clear the annotations
    currentAnnotations = [];

    //Fetch markdown from the server
    fetch('/markdown',
        {
            method: 'POST',
            headers:
                {
                    'Content-Type': 'application/json'
                },
            body: JSON.stringify({message: orionText})
        })
        .then(response => response.json())
        .then(jmarkdown => {
            // Remove the in-progress message
            lastMessage.querySelector('.message-status').remove();

            // Update the message with the final message
            lastMessage.querySelector('.message').innerHTML = jmarkdown.message;

            // Process the new message for code blocks and add copy buttons
            processCodeBlocks(lastMessage);

            // Scroll to the bottom of the chat area
            chatArea.scrollTop = chatArea.scrollHeight;

        }).catch(error => {
        console.error('Error:', error);
        alert('Failed to convert message to markdown: ' + error);
    });
});

orionEventsSource.addEventListener('message.annotation.created', function (event) {
    // This event is triggered when Orion creates an annotation for the message.
    // Should be saved for later use.  When message is completed, the annotations should be added to the message

    currentAnnotations.push(JSON.parse(event.data));
});

orionEventsSource.addEventListener('upload.file.requested', function (event) {
    // This event is triggered when Orion requests a file upload from the user

    // Get the chat area
    const chatArea = document.getElementById('chat-area');

    const jdata = JSON.parse(event.data);
    const filePath = jdata.file_path;

    console.log('Requested upload file:', filePath);
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

async function playAudioFilesSequentially(index = 0) {
    const userID = localStorage.getItem('user_id');

    const speechEndpoint = '/speech/' + index;

    // Fetch the audio files from the server
    await fetch(speechEndpoint, {
        method: 'GET',
        headers: {
            'X-User-Id': userID
        }
    }).then(response => response.blob())
        .then(blob => {
                let audio = document.getElementById('audio');
                audio.src = URL.createObjectURL(blob);
                audio.preload = 'none';
                audio.play();

                // When the audio has finished playing, play the next audio file
                audio.onended = function () {
                    playAudioFilesSequentially(index + 1);
                };
            }
        ).catch(error => {
            console.error('Error:', error);
            alert('Failed to play audio: ' + error);
        });
}

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
                addMessageToChat(messageInput, Files).then(() => {
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

document.addEventListener('DOMContentLoaded', function () {
    // Get the user id from local storage
    const userID = localStorage.getItem('user_id');
    if (userID === null) {
        console.log('User not found in local storage');

        // Redirect to the login page
        window.location.href = '/login.html';
    }

    // Get the chat area
    const chatArea = document.getElementById('chat-area');

    // Fetch the chat messages from the server
    fetch('/chat_history?markdown=true', {
        method: 'GET',
        headers: {
            'X-User-Id': userID,
            'Content-Type': 'text/plain charset=utf-8'
        }
    }).then(response => response.json())
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
                processCodeBlocks(newMessage);
            });

            // Scroll to the bottom of the chat area
            chatArea.scrollTop = chatArea.scrollHeight;

        }).catch(error => {
        console.error('Error:', error);
        alert('Failed to fetch chat history: ' + error);
    });
});

// Function to process the newly added message for code blocks and add copy buttons
function processCodeBlocks(newMessage) {
    const codeBlocks = newMessage.querySelectorAll('pre code');
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
    const codeElements = newMessage.querySelectorAll('code');
    if (codeElements.length > 0) {
        codeElements.forEach(function (code) {
            hljs.highlightElement(code);
        });
    } else {
        console.warn('No <code> elements found within the new message for highlighting.');
    }
}

// Function to create html element for an orion message
function createOrionMessage(message) {
    // Create a new div element for the message-container
    const messageContainer = document.createElement('div');
    messageContainer.className = 'orion-message-container';

    // Create a new div element for the image of the speaker
    const image = document.createElement('img');
    image.src = 'orion.svg';
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

// Function to create html element for a user message
function createUserMessage(message) {
    // Create a new div element for the message-container
    const messageContainer = document.createElement('div');
    messageContainer.className = 'user-message-container';

    // Create a new div element for the image of the speaker
    const image = document.createElement('img');
    image.src = 'user.svg';
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

// Function to add a message to the chat area
async function addMessageToChat(message, files) {
    if (message.trim() !== '') {
        const chatArea = document.getElementById('chat-area');

        // create json object for the message
        const jmessage = {
            message: message,
        };

        //Fetch markdown from the server
        await fetch('/markdown', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify(jmessage)
        })
            .then(response => response.json())
            .then(jmarkdown => {
                const chatArea = document.getElementById('chat-area');

                // get message from markdown
                const markdown = jmarkdown.message;

                // Create a new message for the user
                const newMessage = createUserMessage(markdown);

                // Add the new message to the chat area
                chatArea.appendChild(newMessage);

                // Scroll to the bottom of the chat area
                chatArea.scrollTop = chatArea.scrollHeight;

            }).catch(error => {
                console.error('Error:', error);
                alert('Failed to convert message to markdown: ' + error);
            });

        // Create a body that contains the message and the files
        const body = {
            message: message,
            files: [...files] // Convert the FileList to an array
        };

        // Send the message to the server
        await fetch('/send_message?markdown=true', {
            method: 'POST',
            headers: {
                'X-User-Id': localStorage.getItem('user_id'),
                'Content-Type': 'application/json'
            },
            body: JSON.stringify(body)
        }).then(response => {
            if (response.ok) {
                console.log('Message sent successfully');
            } else {
                console.error('Failed to send message:', response.statusText);
                alert('Failed to send message: ' + response.statusText);
            }
        }).catch(error => {
            console.error('Error:', error);
            alert('Failed to send message: ' + error);
        });
    }
}