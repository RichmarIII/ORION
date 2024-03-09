document.getElementById('send-button').addEventListener('click', function()
{
    var messageInput = document.getElementById('message-input').value;
    var fileInput = document.getElementById('file-input').files;
    if(messageInput.trim() !== '' || fileInput.length > 0)
    {
        addMessageToChat(messageInput, fileInput);
        document.getElementById('message-input').value = '';
        document.getElementById('file-input').value = '';
    }
});

// Many browsers do not support playing audio files without user interaction.
// As a workaround, when the user touches the screen, we will play a silent audio file.
// This will allow us to play audio files later without user interaction.
document.addEventListener('touchstart', function()
{
    // Get the audio element
    let audio = document.getElementById('audio')

    // This is a tiny MP3 file that is silent and extremely short
    audio.src = "data:audio/mpeg;base64,SUQzBAAAAAABEVRYWFgAAAAtAAADY29tbWVudABCaWdTb3VuZEJhbmsuY29tIC8gTGFTb25vdGhlcXVlLm9yZwBURU5DAAAAHQAAA1N3aXRjaCBQbHVzIMKpIE5DSCBTb2Z0d2FyZQBUSVQyAAAABgAAAzIyMzUAVFNTRQAAAA8AAANMYXZmNTcuODMuMTAwAAAAAAAAAAAAAAD/80DEAAAAA0gAAAAATEFNRTMuMTAwVVVVVVVVVVVVVUxBTUUzLjEwMFVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVf/zQsRbAAADSAAAAABVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVf/zQMSkAAADSAAAAABVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV";
    audio.play();

    // Remove the event listener so that the audio is only played once
    document.removeEventListener('touchstart', arguments.callee);
});

async function playAudioFilesSequentially(index = 0)
{
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
            audio.onended = function() {
                playAudioFilesSequentially(index + 1);
            };
        }
    ).catch(error => {
        console.error('Error:', error);
        alert('Failed to play audio: ' + error);
    } );
}

// Add event listener to text input to send message on pressing enter
document.getElementById('message-input').addEventListener('keypress', function(e)
{
    if (e.key === 'Enter')
    {
        var messageInput = document.getElementById('message-input').value;
        var fileInput = document.getElementById('file-input').files;
        if(messageInput.trim() !== '' || fileInput.length > 0)
        {
            addMessageToChat(messageInput, fileInput);
            document.getElementById('message-input').value = '';
            document.getElementById('file-input').value = '';
        }
    }
});

document.getElementById('new-chat-button').addEventListener('click', function()
{
    // Implement the logic to handle new chat creation here
    alert('New chat functionality to be implemented');
});

document.addEventListener('DOMContentLoaded', function()
{
    // Get the user id from local storage
    var userID = localStorage.getItem('user_id');
    if (userID === null)
    {
        console.log('User not found in local storage');

        // Redirect to the login page
        window.location.href = '/login.html';
    }

    // Get the chat area
    var chatArea = document.getElementById('chat-area');

    // Fetch the chat messages from the server
    fetch('/chat_history?markdown=true', {
        method: 'GET',
        headers: {
            'X-User-Id': userID,
            'Content-Type': 'text/plain charset=utf-8'
        }
    }).then(response => response.json())
    .then(messages => {
        var chatArea = document.getElementById('chat-area');

        // Loop through the messages and add them to the chat area
        for (var i = 0; i < messages.length; i++)
        {
            if (messages[i].role === 'user')
            {
                // Create a new message for the user
                var newMessage = createUserMessage(messages[i].message);
                // Add the new message to the chat area
                chatArea.appendChild(newMessage);
            }
            else
            {
                // Create a new message for orion
                var newMessage = createOrionMessage(messages[i].message);
                // Add the new message to the chat area
                chatArea.appendChild(newMessage);
            }
        }

        // Process the new messages for code blocks and add copy buttons
        var newMessages = chatArea.querySelectorAll('.orion-message-container');
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
function processCodeBlocks(newMessage)
{
    var codeBlocks = newMessage.querySelectorAll('pre code');
    if (codeBlocks.length > 0)
    {
        codeBlocks.forEach(codeBlock =>
        {
            // Add copy button to each code block
            var copyButton = document.createElement('button');
            copyButton.textContent = 'Copy';
            copyButton.className = 'copy-button';
            copyButton.addEventListener('click', function()
            {
                navigator.clipboard.writeText(codeBlock.textContent).then(function()
                {
                    console.log('Code copied to clipboard');
                }, function(err)
                {
                    console.error('Failed to copy code to clipboard', err);
                });
            });
            codeBlock.parentNode.insertBefore(copyButton, codeBlock);
        });
    }

    // Highlight all code elements within the newMessage
    var codeElements = newMessage.querySelectorAll('code');
    if (codeElements.length > 0) {
        codeElements.forEach(function(code) {
            hljs.highlightElement(code);
        });
    } else {
        console.warn('No <code> elements found within the new message for highlighting.');
    }
}

// Function to create html element for an orion message
function createOrionMessage(message)
{
    // Create a new div element for the message-container
    var messageContainer = document.createElement('div');
    messageContainer.className = 'orion-message-container';

    // Create a new div element for the image of the speaker
    var image = document.createElement('img');
    image.src = 'orion.svg';
    image.alt = 'Orion';
    image.className = 'image';

    // Create a new div element for the message
    var messageDiv = document.createElement('div');
    messageDiv.className = 'message';
    messageDiv.innerHTML = message;

    // Add the image and message to the message-container
    messageContainer.appendChild(image);
    messageContainer.appendChild(messageDiv);

    return messageContainer;
}

// Function to create html element for a user message
function createUserMessage(message)
{
    // Create a new div element for the message-container
    var messageContainer = document.createElement('div');
    messageContainer.className = 'user-message-container';

    // Create a new div element for the image of the speaker
    var image = document.createElement('img');
    image.src = 'user.svg';
    image.alt = 'User';
    image.className = 'image';

    // Create a new div element for the message
    var messageDiv = document.createElement('div');
    messageDiv.className = 'message';
    messageDiv.innerHTML = message;

    // Add the image and message to the message-container
    messageContainer.appendChild(image);
    messageContainer.appendChild(messageDiv);

    return messageContainer;
}

// Function to add a message to the chat area
async function addMessageToChat(message, files)
{
    if (message.trim() !== '')
    {
        let filesToPlay = [];
        var chatArea = document.getElementById('chat-area');

        // create json object for the message
        var jmessage = {
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
            var chatArea = document.getElementById('chat-area');

            // get message from markdown
            const markdown = jmarkdown.message;

            // Create a new message for the user
            var newMessage = createUserMessage(markdown);

            // Add the new message to the chat area
            chatArea.appendChild(newMessage);

            // Process the new message for code blocks and add copy buttons
            processCodeBlocks(newMessage);

            // Scroll to the bottom of the chat area
            chatArea.scrollTop = chatArea.scrollHeight;

        }).catch(error => {
            console.error('Error:', error);
            alert('Failed to convert message to markdown: ' + error);
        });

        // Send the message to the server
        await fetch('/send_message?markdown=true', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
                'X-User-Id': localStorage.getItem('user_id')
            },
            body: JSON.stringify(jmessage)
        }).then(response => response.json())
        .then(jmessage => {
            var chatArea = document.getElementById('chat-area');

            const message = jmessage.message;

            // Create a new message for orion
            var newMessage = createOrionMessage(message);

            // Add the new message to the chat area
            chatArea.appendChild(newMessage);

            // Process the new message for code blocks and add copy buttons
            processCodeBlocks(newMessage);

            // Scroll to the bottom of the chat area
            chatArea.scrollTop = chatArea.scrollHeight;

            // Now lets speak the message
            fetch('/speak?format=mp3', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                    'X-User-Id': localStorage.getItem('user_id')
                },
                body: JSON.stringify(jmessage)
            }).then(response =>
            {
                playAudioFilesSequentially();
                
            }).catch(error => {
                console.error('Error:', error);
                alert('Failed to speak message: ' + error);
            });

        }).catch(error => {
            console.error('Error:', error);
            alert('Failed to send message: ' + error);
        });
    }
}