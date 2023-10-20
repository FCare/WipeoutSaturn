import bpy
import array as arr


def get_i32(f):
    return int.from_bytes( f.read(4), byteorder='big', signed = True)

def get_i16(f):
    return int.from_bytes( f.read(2), byteorder='big', signed = True)

def pad(f, off):
    f.seek(off, 1)

def read_some_data(context, filepath, use_some_setting):
    print("importing from "+filepath)
    with open(filepath, mode="rb") as f:
        chunk = f.read(16)
        while chunk:
            name = chunk.decode("utf-8", "ignore")
            mesh = bpy.data.meshes.new( name ) # create a new mesh
            print("Generating mesh "+name)
            nb_vertices = get_i16(f)
            pad(f,2)
            pad(f,4)
            nb_normals = get_i16(f)
            pad(f,2)
            pad(f,4)
            nb_primitives = get_i16(f)
            pad(f,2)
            pad(f,4)
            pad(f,4)
            pad(f,4)
            pad(f,4)
            extent = get_i32(f)
            flags = get_i16(f)
            pad(f,2)
            pad(f,4)
            pad(f,18) #rot Matrix
            pad(f,2)
            originX = get_i32(f)
            originY = get_i32(f)
            originZ = get_i32(f)
            pad(f,18)
            pad(f,2)
            pad(f,12)
            pad(f,2)
            pad(f,2)
            pad(f,4)
            pad(f,4)
            pad(f,4)
            vertices = []
            for i in range(nb_vertices):
                x = get_i16(f)
                y = get_i16(f)
                z = get_i16(f)
                print(str(x)+ " " + str(y) + " " + str(z))
                vertices.append((x,y,z))
                pad(f,2)
            normals = []
            for i in range(nb_normals):
                normals.append((get_i16(f),get_i16(f),get_i16(f)))
                pad(f,2)

            primitives = []
            for i in range(nb_primitives):
                prm_type = get_i16(f)
                # print("type is "+ str(prm_type))
                prm_flag = get_i16(f)
                match prm_type:
                    case 1: #F3
                        primitives.append((get_i16(f),get_i16(f),get_i16(f)))
                        pad(f, 6)
                    case 2: #FT3
                        primitives.append((get_i16(f),get_i16(f),get_i16(f)))
                        pad(f, 18)
                    case 3: #F4
                        point = []
                        point.append(get_i16(f))
                        point.append(get_i16(f))
                        point.append(get_i16(f))
                        point.append(get_i16(f))
                        primitives.append((point[0],point[1],point[3],point[2]))
                        pad(f, 4)
                    case 4: #FT4
                        point = []
                        point.append(get_i16(f))
                        point.append(get_i16(f))
                        point.append(get_i16(f))
                        point.append(get_i16(f))
                        primitives.append((point[0],point[1],point[3],point[2]))
                        pad(f, 20)
                    case 5: #G3
                        primitives.append((get_i16(f),get_i16(f),get_i16(f)))
                        pad(f, 14)
                    case 6: #GT3
                        primitives.append((get_i16(f),get_i16(f),get_i16(f)))
                        pad(f, 26)
                    case 7: #G4
                        point = []
                        point.append(get_i16(f))
                        point.append(get_i16(f))
                        point.append(get_i16(f))
                        point.append(get_i16(f))
                        primitives.append((point[0],point[1],point[3],point[2]))
                        pad(f, 16)
                    case 8: #GT4
                        point = []
                        point.append(get_i16(f))
                        point.append(get_i16(f))
                        point.append(get_i16(f))
                        point.append(get_i16(f))
                        primitives.append((point[0],point[1],point[3],point[2]))
                        pad(f, 32)
                    #case 9: #LF2 - never seen
                    #case 10 | 11: #TSPR
                    case 10 | 11: #BSPR
                        pad(f, 12)
                    case 12: #LSF3
                        primitives.append((get_i16(f),get_i16(f),get_i16(f)))
                        pad(f, 6)
                    case 13: #LSFT3
                        primitives.append((get_i16(f),get_i16(f),get_i16(f)))
                        pad(f, 18)
                    case 14: #LSF4
                        point = []
                        point.append(get_i16(f))
                        point.append(get_i16(f))
                        point.append(get_i16(f))
                        point.append(get_i16(f))
                        primitives.append((point[0],point[1],point[3],point[2]))
                        pad(f, 8)
                    case 15: #LSFT4
                        point = []
                        point.append(get_i16(f))
                        point.append(get_i16(f))
                        point.append(get_i16(f))
                        point.append(get_i16(f))
                        primitives.append((point[0],point[1],point[3],point[2]))
                        pad(f, 20)
                    case 16: #LSG3
                        primitives.append((get_i16(f),get_i16(f),get_i16(f)))
                        pad(f, 18)
                    case 17: #LSGT3
                        primitives.append((get_i16(f),get_i16(f),get_i16(f)))
                        pad(f, 30)
                    case 18: #LSG4
                        point = []
                        point.append(get_i16(f))
                        point.append(get_i16(f))
                        point.append(get_i16(f))
                        point.append(get_i16(f))
                        primitives.append((point[0],point[1],point[3],point[2]))
                        pad(f, 24)
                    case 19: #LSGT4
                        point = []
                        point.append(get_i16(f))
                        point.append(get_i16(f))
                        point.append(get_i16(f))
                        point.append(get_i16(f))
                        primitives.append((point[0],point[1],point[3],point[2]))
                        pad(f, 38)
                    case 20: #SPLINE
                        pad(f, 52)
                    case 21: #INFINITE_LIGHT
                        pad(f, 12)
                    case 22: #POINT_LIGHT
                        pad(f, 24)
                    case 23: #SPOT_LIGHT
                        pad(f, 36)
                    case _:
                        print("Error unsupported primitive")

            chunk = f.read(16)
            mesh.from_pydata(vertices, [], primitives)
            mesh.update()
            object = bpy.data.objects.new(name, mesh)
            collection = bpy.data.collections.new(name)
            bpy.context.scene.collection.children.link(collection)
            collection.objects.link(object)
    return {'FINISHED'}


# ImportHelper is a helper class, defines filename and
# invoke() function which calls the file selector.
from bpy_extras.io_utils import ImportHelper
from bpy.props import StringProperty, BoolProperty, EnumProperty
from bpy.types import Operator


class ImportShipData(Operator, ImportHelper):
    """This appears in the tooltip of the operator and in the generated docs"""
    bl_idname = "import_wipeout.ships"  # important since its how bpy.ops.import_test.some_data is constructed
    bl_label = "Import Wipeout Ships"

    # ImportHelper mixin class uses this
    filename_ext = ".prm"

    filter_glob: StringProperty(
        default="*.prm",
        options={'HIDDEN'},
        maxlen=255,  # Max internal buffer length, longer would be clamped.
    )

    # List of operator properties, the attributes will be assigned
    # to the class instance from the operator settings before calling.
    use_setting: BoolProperty(
         name="Example Boolean",
         description="Example Tooltip",
         default=True,
    )

    # type: EnumProperty(
    #     name="Example Enum",
    #     description="Choose between two items",
    #     items=(
    #         ('OPT_A', "First Option", "Description one"),
    #         ('OPT_B', "Second Option", "Description two"),
    #     ),
    #     default='OPT_A',
    # )

    def execute(self, context):
        return read_some_data(context, self.filepath, self.use_setting)


# Only needed if you want to add into a dynamic menu
def menu_func_import(self, context):
    self.layout.operator(ImportShipData.bl_idname, text="Wipeout ship Import")

# Register and add to the "file selector" menu (required to use F3 search "Text Import Operator" for quick access)
def register():
    bpy.utils.register_class(ImportShipData)
    bpy.types.TOPBAR_MT_file_import.append(menu_func_import)


def unregister():
    bpy.utils.unregister_class(ImportShipData)
    bpy.types.TOPBAR_MT_file_import.remove(menu_func_import)


if __name__ == "__main__":
    register()

    # test call
    bpy.ops.import_wipeout.ships('INVOKE_DEFAULT')
