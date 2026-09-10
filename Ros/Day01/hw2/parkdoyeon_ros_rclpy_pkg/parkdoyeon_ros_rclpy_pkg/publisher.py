import rclpy

from rclpy.node import Node

from std_msgs.msg import Int32
from std_msgs.msg import String
from std_msgs.msg import Float32
from std_msgs.msg import Bool


class PublisherNode(Node):

    def __init__(self):
        super().__init__('py_publisher')

        self.int_publisher = self.create_publisher(
            Int32,
            'py_int_topic',
            10
        )

        self.string_publisher = self.create_publisher(
            String,
            'py_string_topic',
            10
        )

        self.float_publisher = self.create_publisher(
            Float32,
            'py_float_topic',
            10
        )

        self.bool_publisher = self.create_publisher(
            Bool,
            'py_bool_topic',
            10
        )

        self.timer = self.create_timer(
            1.0,
            self.timer_callback
        )

        self.count = 0

    def timer_callback(self):
        int_msg = Int32()
        string_msg = String()
        float_msg = Float32()
        bool_msg = Bool()

        int_msg.data = self.count
        string_msg.data = 'Hello from Python'
        float_msg.data = 2.71
        bool_msg.data = self.count % 2 == 0

        self.int_publisher.publish(int_msg)
        self.string_publisher.publish(string_msg)
        self.float_publisher.publish(float_msg)
        self.bool_publisher.publish(bool_msg)

        self.get_logger().info(
            f'int: {int_msg.data}, '
            f'string: {string_msg.data}, '
            f'float: {float_msg.data:.2f}, '
            f'bool: {bool_msg.data}'
        )

        self.count += 1


def main(args=None):
    rclpy.init(args=args)

    node = PublisherNode()

    rclpy.spin(node)

    node.destroy_node()

    rclpy.shutdown()


if __name__ == '__main__':
    main()