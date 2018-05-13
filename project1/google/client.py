import os
import sys
import threading
import socket
import unittest
import time

from google.cloud import pubsub
from google.api_core.exceptions import AlreadyExists, NotFound

publisher = pubsub.PublisherClient()
subscriber = pubsub.SubscriberClient()

# Uniquely ID this client by hostname and PID
sub_id = '{}{}'.format(socket.gethostname(), os.getpid())

if len(sys.argv) > 1:
    project_id = sys.argv[1]
else:
    project_id = os.getenv('PUBSUB_PROJECT_ID', default='csci-5105-project-1')


def format_sub(topic):
    return '{}_{}'.format(subscriber.subscription_path(project_id, topic), sub_id)


def create_topic(topic):
    try:
        topic = publisher.topic_path(project_id, topic)
        publisher.create_topic(topic)
    except AlreadyExists as e:
        print(e)


def delete_topic(topic):
    try:
        topic = publisher.topic_path(project_id, topic)
        publisher.delete_topic(topic)
    except AlreadyExists:
        pass


def subscribe(topic):
    try:
        topic_full = publisher.topic_path(project_id, topic)
        subscription_full = format_sub(topic)
        subscription = subscriber.create_subscription(
            subscription_full, topic_full)
        subscription = subscriber.subscribe(subscription_full)
        future = subscription.open(callback)
    except (AlreadyExists, NotFound) as e:
        print(e)


def unsubscribe(topic):
    try:
        subscription = format_sub(topic)
        subscriber.delete_subscription(subscription)
    except NotFound as e:
        print(e)


def publish(topic, message):
    try:
        topic_full = publisher.topic_path(project_id, topic)
        publisher.publish(topic_full, message.encode())
    except NotFound as e:
        print(e)


def callback(message):
    print('*** New message:', message.data.decode())
    message.ack()


def cleanup_subs():
    'Delete all subscriptions associated with this client'
    project = subscriber.project_path(project_id)
    for sub in subscriber.list_subscriptions(project):
        if sub.name.split('_')[-1] == sub_id:
            subscriber.delete_subscription(sub.name)


class Unit_Test(unittest.TestCase):

    def test_a_create_topic(self):
        create_topic('test')
        output = sys.stdout.getvalue().strip()
        self.assertEqual(output, '')  # no error message
        create_topic('test')
        output = sys.stdout.getvalue().strip()
        # Duplicate create shows error
        self.assertEqual(output, '409 Topic already exists')

    def test_b_subscribe(self):
        subscribe('test')
        output = sys.stdout.getvalue().strip()
        self.assertEqual(output, '')  # no error message
        subscribe('does-not-exist')
        output = sys.stdout.getvalue().strip()
        # shows error message
        self.assertEqual(output, '404 Subscription topic does not exist')

    def test_c_publish(self):
        publish('test', 'test successfull!')
        time.sleep(2)
        output = sys.stdout.getvalue().strip()
        self.assertEqual(output, '*** New message: test successfull!')

    def test_d_unsubscribe(self):
        unsubscribe('test')  # no error message
        output = sys.stdout.getvalue().strip()
        self.assertEqual(output, '')  # no error message
        # no error message because we have unsubed at this point
        subscribe('test')
        output = sys.stdout.getvalue().strip()
        self.assertEqual(output, '')  # no error message
        unsubscribe('does-not-exist')
        output = sys.stdout.getvalue().strip()
        # shows error message
        self.assertEqual(output, '404 Subscription does not exist')

    @classmethod
    def tearDownClass(cls):
        delete_topic('test')


if __name__ == '__main__':
    host = os.getenv('PUBSUB_EMULATOR_HOST')
    print('Connected to project {} at {}'.format(project_id, host))

    # Run unit tests
    if len(sys.argv) > 1 and sys.argv[1] == 'test':
        sys.argv = sys.argv[:1]
        unittest.main(buffer=True)

    while True:
        cmd = []
        while cmd == []:
            cmd = input('pubsub> ').split()

        if cmd[0] == 'q' or cmd[0] == 'Q':
            # Don't keep our subscriptions around after we exit
            cleanup_subs()
            sys.exit(0)

        # create_topic command
        # usage: create_topic <topic>
        elif cmd[0] == 'create_topic':
            if len(cmd) < 2:
                print('You must also provide the topic to create')
            elif len(cmd) > 2:
                print('The topic name may not contain spaces')
            else:
                topic = cmd[1].strip('\'').strip('"')
                create_topic(topic)

        # subscribe command
        # usage: subscribe <topic>
        elif cmd[0] == 'subscribe':
            if len(cmd) < 2:
                print('You must also provide the topic to subscribe to')
            else:
                topic = cmd[1].strip('\'').strip('"')
                subscribe(topic)

        # unsubscribe command
        # usage: unsubscribe <topic>
        elif cmd[0] == 'unsubscribe':
            if len(cmd) < 2:
                print('You must also provide the topic to unsubscribe from')
            else:
                topic = cmd[1].strip('\'').strip('"')
                unsubscribe(topic)

        # publish command
        # usage: publish <topic> <message>
        elif cmd[0] == 'publish':
            if len(cmd) < 3:
                print('You must also provide the topic and message to publish')
            else:
                topic = cmd[1].strip('\'').strip('"')
                message = ' '.join(cmd[2:]).strip('\'').strip('"')
                publish(topic, message)

        else:
            print('Command not recognized')
