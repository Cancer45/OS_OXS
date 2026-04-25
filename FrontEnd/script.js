const API_BASE_URL = '127.0.0.1:8888';

actionBtn.addEventListener('click', async () => {
    const userName = nameInput.value.trim();
    const userRole = roleSelect.value;

    if (!userName) {
        alert("Please enter a name first.");
        return;
    }

    // 1. Create the data object
    const userData = {
        name: userName,
        role: userRole
    };

    try {
        // 2. Use fetch to send the POST request
        const response = await fetch(`${API_BASE_URL}/users`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify(userData) // Convert JS object to JSON string
        });

        // 3. Check if the server responded successfully (status 200-299)
        if (response.ok) {
            const result = await response.json();
            console.log('Success:', result);
            alert(`Success! ${userName} was saved to the server.`);
            nameInput.value = '';
        } else {
            console.error('Server Error:', response.statusText);
            alert('The server rejected the request.');
        }

    } catch (error) {
        // Handle network errors (e.g., DNS issues or the server is down)
        console.error('Network Error:', error);
        alert('Could not connect to the server.');
    }
});
