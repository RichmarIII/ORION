document.addEventListener('DOMContentLoaded', function()
{
    
});

document.getElementById('login-form').addEventListener('submit', function(event) {
    event.preventDefault(); // Prevent the default form submission

    // Get the form data
    var username = document.getElementById('username').value;
    var password = document.getElementById('password').value;

    // Create the JSON object
    var data = {
        username: username,
        password: password
    };

    if (username === '' || password === '') {
        alert('Please fill in all fields');
        return;
    }

    // Send the data using Fetch API or XMLHttpRequest
    fetch('/login', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify(data)
    })
    .then(response => response.json())
    .then(data => {
        console.log('Success:', data);

        // Navigate to the orion page
        window.location.href = '/orion.html';
    })
    .catch((error) => {
        console.error('Error:', error);
        alert('Invalid username or password');
    });
});