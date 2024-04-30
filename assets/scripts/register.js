document.addEventListener('DOMContentLoaded', function()
{
    
});

document.getElementById('register-form').addEventListener('submit', function(event) {
    event.preventDefault(); // Prevent the default form submission

    // Get the form data
    var username = document.getElementById('username').value;
    var password = document.getElementById('password').value;
    var passwordConfirm = document.getElementById('password-confirm').value;

    // Check if the passwords match
    if (password !== passwordConfirm) {
        alert('Passwords do not match');
        return;
    }

    // Create the JSON object
    var data = {
        username: username,
        password: password
    };

    // Send the data using Fetch API or XMLHttpRequest
    fetch('/register', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify(data)
    })
    .then(response => response.json())
    .then(data => {
        console.log('Success:', data);

        // Store the user_id in local storage
        localStorage.setItem('user_id', data.user_id);

        // Navigate to the orion page
        window.location.href = '/assets/html/orion.html';
    })
    .catch((error) => {
        console.error('Error:', error);
        alert('Invalid username or password');
    });
});