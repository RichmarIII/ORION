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
            
            var chatArea = document.getElementById('chat-area');
            var newMessage = document.createElement('div');
            
            // Parse the json object
            var data = JSON.parse(e.data);
            
            newMessage.innerHTML = data.message;
            chatArea.appendChild(newMessage);

            // Call function to process the newly added message for code blocks and add copy buttons
            processCodeBlocks(newMessage);

            // Scroll to the bottom of the chat area
            chatArea.scrollTop = chatArea.scrollHeight;

        },false);
    }
    else
    {
        console.log("Your browser does not support Server-Sent Events.");
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
        var chatArea = document.getElementById('chat-area');
        var message_json = JSON.stringify({ message: message });

        // Fetch markdown from the server
        await fetch('/message_to_markdown', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: message_json
        })
        .then(response => response.json())
        .then(data => {
            var newMessage = document.createElement('div');
            newMessage.innerHTML = data.message;
            chatArea.appendChild(newMessage);
            processCodeBlocks(newMessage);
            chatArea.scrollTop = chatArea.scrollHeight;
        }).catch(error => {
            console.error('Error:', error);
            alert('Failed to convert message to markdown: ' + error);
        });

        // Send the message to the server
        await fetch('/send_message', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: message_json
        });
    }
}
