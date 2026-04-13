import matplotlib.pyplot as plt
import numpy as np

from pytransform3d import rotations as pr
from pytransform3d import transformations as pt
from pytransform3d.transform_manager import TransformManager


cc2flume = pt.transform_from(
    pr.matrix_from_euler(np.array([0.0, 0.0, 0.0]), 0, 1, 2, True),
    np.array([0.0, 0.5, 1.0]),
)
cc2flume2 = pt.transform_from(
    pr.matrix_from_euler(np.array([0.0, 0.0, 0.0]), 0, 1, 2, True),
    np.array([0.0, 0.5, 12.0]),
)
tof2cc =  pt.transform_from(
    pr.matrix_from_euler(np.array([0, 0, - 0.5 * np.pi]), 0, 1, 2, True),
    np.array([0.0, 0.1, -0.2]),
)
object2tof = pt.transform_from(
    pr.matrix_from_euler(np.array([0.0, 0.0, 0.0]), 0, 1, 2, True),
    np.array([0.0, 0.0, -0.5]),
)

tm = TransformManager()
tm.add_transform("camera_cart", "flume", cc2flume)
tm.add_transform("ToF_sensor", "camera_cart", tof2cc)
tm.add_transform("object", "ToF_sensor", object2tof)


# cc2flume2=pt.translate_transform(cc2flume, np.array([0.0, 3.0, 0.0]))
# tm.add_transform("camera_cart", "flume", cc2flume2)

# flume2cc = tm.get_transform("object", "flume")

ax = tm.plot_frames_in("flume", s=0.1)
ax.set_xlim((-0.25, 0.75))
ax.set_ylim((0.0, 2.0))
ax.set_zlim((0.0, 1.0))
plt.show()

