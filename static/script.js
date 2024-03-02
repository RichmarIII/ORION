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

async function playAudioFilesSequentially(files, index = 0)
{
    if (index < files.length) {
        const fileName = files[index].file;

        // Create an audio element and set its source to the audio file
        let audio = new Audio(`/audio/${fileName}`);
        audio.preload = 'none';
        
        // Play the audio file
        await audio.play();

        return new Promise(resolve => {
            // Once the audio ends, play the next one
            audio.onended = () => {
                resolve(playAudioFilesSequentially(files, index + 1));
            };
        });
    }
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

// On load, check if the browser supports Server-Sent Events
document.addEventListener('DOMContentLoaded', function()
{
    // Check if the browser supports Server-Sent Events
    if (!!window.EventSource)
    {
        var source = new EventSource('/events');

        // Listen for "ai_message_ready" events. data is a json object containing the message. Format of the json object is {message: 'message'}
        source.addEventListener('ai_message_ready', function(e)
        {
            console.log('ai_message_ready', e.data);
        },false);
    }
    else
    {
        console.log("Your browser does not support Server-Sent Events.");
    }

    // Check for existing Orion ID in the local storage
    var orion_id = localStorage.getItem('orion_id');
    if (orion_id)
    {
        console.log('Orion ID found in local storage:', orion_id);
    }
    else
    {
        console.log('No Orion ID found in local storage');
    }

    if (orion_id)
    {
        // Orion ID needs to be sent to the server via query parameter
        // Fetch the Orion details from the server
        fetch('/create_orion?id=' + orion_id, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            }
        }).then(response => response.json()).then(data => {
            console.log('Orion details:', data);
            localStorage.setItem('orion_id', data.id);
        }).catch(error => {
            console.error('Error:', error);
            alert('Failed to get Orion details: ' + error);
        });
    }
    else
    {
        // Fetch the Orion details from the server
        fetch('/create_orion', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            }
        }).then(response => response.json()).then(data => {
            console.log('Orion details:', data);
            localStorage.setItem('orion_id', data.id);
        }).catch(error => {
            console.error('Error:', error);
            alert('Failed to get Orion details: ' + error);
        });
    }
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

async function addMessageToChat(message, files)
{
    if (message.trim() !== '')
    {
        let filesToPlay = [];
        var chatArea = document.getElementById('chat-area');

        //Fetch markdown from the server
        await fetch('/markdown', {
            method: 'POST',
            headers: {
                'Content-Type': 'text/plain'
            },
            body: message
        })
        .then(response => response.text())
        .then(markdown => {
            var newMessage = document.createElement('div');
            newMessage.innerHTML = markdown;
            chatArea.appendChild(newMessage);
            processCodeBlocks(newMessage);
            chatArea.scrollTop = chatArea.scrollHeight;
        }).catch(error => {
            console.error('Error:', error);
            alert('Failed to convert message to markdown: ' + error);
        });

        // Send the message to the server
        await fetch('/send_message?markdown=true', {
            method: 'POST',
            headers: {
                'Content-Type': 'text/plain',
                'X-Orion-Id': localStorage.getItem('orion_id')
            },
            body: message
        }).then(response => response.text())
        .then(message => {
            var chatArea = document.getElementById('chat-area');
            var newMessage = document.createElement('div');

            // Add the new message to the chat area
            newMessage.innerHTML = message;
            chatArea.appendChild(newMessage);

            // Process the new message for code blocks and add copy buttons
            processCodeBlocks(newMessage);

            // Scroll to the bottom of the chat area
            chatArea.scrollTop = chatArea.scrollHeight;

            // Now lets speak the message
            fetch('/speak?format=opus', {
                method: 'POST',
                headers: {
                    'Content-Type': 'text/plain',
                    'Accept': 'application/json',
                    'X-Orion-Id': localStorage.getItem('orion_id')
                },
                body: message
            }).then(response => response.json()).then(data =>
            {
                let files = data.files; // Array of audio files to play

                if (files.length > 0) {
                    playAudioFilesSequentially(files);
                }
                
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
