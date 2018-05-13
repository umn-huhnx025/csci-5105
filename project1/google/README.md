# Google PubSub

### Set up a Virtual Environment
This will set up a fresh Python3 environment where we can install all the dependencies we need.

```
$ virtualenv -p python3 venv
$ . venv/bin/activate
$
$ # To go back to normal...
$ deactivate
```

### Install Python Dependencies
```
$ pip install -r requirements.txt
```

### Run the Emulator
This requires installing the Google Cloud SDK and the pubsub-emulator component. Details for that are in the Project1 handout.

```
$ gcloud beta emulators pubsub start
```

### Set Environment Variables
```
$ export GOOGLE_APPLICATION_CREDENTIALS="/path/to/json/creds"
$ export PUBSUB_PROJECT_ID="csci-5105-project-1"
$ $(gcloud beta emulators pubsub env-init)
```

### Run the client
```
$ python client.py
```
### Run unit tests
```
$ python client.py test
```
