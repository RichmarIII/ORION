document.addEventListener('DOMContentLoaded', function()
{
    // Navigate to the login page if the user is not logged in
    if (!localStorage.getItem('user_id'))
    {
        window.location.href = '/login.html';
    }
    else
    {
        // Load the user's ID from local storage
        var userID = localStorage.getItem('user_id');
        const jsonBody = JSON.stringify({ user_id: userID });

        // Log the user in
        fetch('/login', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: jsonBody
        }).then(response => response.json())
        .then(data =>
        {
            if (!data.user_id)
            {
                window.location.href = '/login.html';
            }
            else
            {
                // Navigate to the orion page if the user is logged in
                window.location.href = '/orion.html';
            }
        }).catch(error => {
            console.error('Error:', error);
            alert('Failed to login: ' + error);
        });
    }
});