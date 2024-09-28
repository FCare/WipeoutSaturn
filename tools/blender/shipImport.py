import bpy
import array as arr
import bmesh


def get_i32(f):
    return int.from_bytes( f.read(4), byteorder='big', signed = True)

def get_u32(f):
    return int.from_bytes( f.read(4), byteorder='big', signed = False)

def get_i16(f):
    return int.from_bytes( f.read(2), byteorder='big', signed = True)

def get_i8(f):
    return int.from_bytes( f.read(1), byteorder='big', signed = True)

def int32torgb(color):
    rgb = []
    for i in range(3):
        rgb.append(color&0xFF)
        color = color >> 8
    rgb.append(0xFF)
    return rgb

def pad(f, off):
    f.seek(off, 1)

def read_some_data(context, filepath, use_some_setting):
    print("importing from "+filepath)
    collection = bpy.data.collections.new("Scene")
    bpy.context.scene.collection.children.link(collection)
    with open(filepath, mode="rb") as f:
        chunk = f.read(16)
        while chunk:
            name = chunk.decode("utf-8", "ignore")
            mesh_data = bpy.data.meshes.new( f"{name}_data" ) # create a new mesh
            mesh_obj =bpy.data.objects.new(name, mesh_data)
            bpy.data.objects.new(name, mesh_data)
            bm = bmesh.new()   # create an empty BMesh
            # add the mesh object into the scene
            collection.objects.link(mesh_obj)
            
            collayer = bm.loops.layers.color.new("Attribute")
            
            
            new_uv = mesh_data.uv_layers.new(name='CMP_UV') # create new UV maps
            print("Generating mesh "+name)
            nb_vertices = get_i16(f)
            pad(f,2)
            pad(f,4)
            nb_normals = get_i16(f)
            pad(f,2)
            pad(f,4)
            nb_faces = get_i16(f)
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
            for i in range(nb_vertices):
                x = get_i16(f) + originX
                y = get_i16(f) + originY
                z = get_i16(f) + originZ
                bm.verts.new((x,y,z))
                pad(f,2)
            bm.verts.ensure_lookup_table()
            normals = []
            for i in range(nb_normals):
                normals.append((get_i16(f),get_i16(f),get_i16(f)))
                pad(f,2)

            faces = []
            for i in range(nb_faces):
                prm_type = get_i16(f)
                # print("type is "+ str(prm_type))
                prm_flag = get_i16(f)
                match prm_type:
                    case 1: #F3
                        face_vertices = [bm.verts[get_i16(f)],bm.verts[get_i16(f)],bm.verts[get_i16(f)]]
                        pad(f, 2)
                        color = int32torgb(get_u32(f))
                        new = bm.faces.new(face_vertices)
                        for loop in new.loops:
                            loop[collayer] = color
                    case 2: #FT3
                        face_vertices = [bm.verts[get_i16(f)],bm.verts[get_i16(f)],bm.verts[get_i16(f)]]
                        texture = get_i16(f)
                        pad(f, 4)
                        face_uv = ((get_i8(f),get_i8(f),get_i8(f),get_i8(f),get_i8(f),get_i8(f)))
                        pad(f, 2)
                        color = int32torgb(get_u32(f))
                        new = bm.faces.new(face_vertices)
                        for loop in new.loops:
                            loop[collayer] = color
                    case 3: #F4
                        point = (get_i16(f),get_i16(f),get_i16(f),get_i16(f))
                        face_vertices = [bm.verts[point[0]],bm.verts[point[1]],bm.verts[point[3]],bm.verts[point[2]]]
                        color = int32torgb(get_u32(f))
                        new = bm.faces.new(face_vertices)
                        for loop in new.loops:
                            loop[collayer] = color
                    case 4: #FT4
                        point = (get_i16(f),get_i16(f),get_i16(f),get_i16(f))
                        face_vertices = [bm.verts[point[0]],bm.verts[point[1]],bm.verts[point[3]],bm.verts[point[2]]]
                        texture = get_i16(f)
                        pad(f, 4)
                        f_uv = (get_i8(f),get_i8(f),get_i8(f),get_i8(f),get_i8(f),get_i8(f), get_i8(f), get_i8(f))
                        face_uv = ((f_uv[0],f_uv[1],f_uv[2],f_uv[3],f_uv[6],f_uv[7],f_uv[4],f_uv[5]))
                        pad(f, 2)
                        color = int32torgb(get_u32(f))
                        new = bm.faces.new(face_vertices)
                        for loop in new.loops:
                            loop[collayer] = color
                    case 5: #G3
                        face_vertices = [bm.verts[get_i16(f)],bm.verts[get_i16(f)],bm.verts[get_i16(f)]]
                        pad(f,2)
                        new = bm.faces.new(face_vertices)
                        color = [0,0,0]
                        color[0] = int32torgb(get_u32(f))
                        color[1] = int32torgb(get_u32(f))
                        color[2] = int32torgb(get_u32(f))
                        for i, loop in enumerate(new.loops):
                            loop[collayer] = color[i]
                    case 6: #GT3
                        face_vertices = [bm.verts[get_i16(f)],bm.verts[get_i16(f)],bm.verts[get_i16(f)]]
                        texture = get_i16(f)
                        pad(f, 4)
                        face_uv = (get_i8(f),get_i8(f),get_i8(f),get_i8(f),get_i8(f),get_i8(f))
                        pad(f,2)
                        new = bm.faces.new(face_vertices)
                        color = [0,0,0]
                        color[0] = int32torgb(get_u32(f))
                        color[1] = int32torgb(get_u32(f))
                        color[2] = int32torgb(get_u32(f))
                        for i, loop in enumerate(new.loops):
                            loop[collayer] = color[i]
                    case 7: #G4
                        point = (get_i16(f),get_i16(f),get_i16(f),get_i16(f))
                        face_vertices = [bm.verts[point[0]],bm.verts[point[1]],bm.verts[point[3]],bm.verts[point[2]]]
                        new = bm.faces.new(face_vertices)
                        color = [0,0,0,0]
                        color[0] = int32torgb(get_u32(f))
                        color[1] = int32torgb(get_u32(f))
                        color[3] = int32torgb(get_u32(f))
                        color[2] = int32torgb(get_u32(f))
                        for i, loop in enumerate(new.loops):
                            loop[collayer] = color[i]
                    case 8: #GT4
                        point = (get_i16(f),get_i16(f),get_i16(f),get_i16(f))
                        face_vertices = [bm.verts[point[0]],bm.verts[point[1]],bm.verts[point[3]],bm.verts[point[2]]]
                        texture = get_i16(f)
                        pad(f, 4)
                        f_uv = (get_i8(f),get_i8(f),get_i8(f),get_i8(f),get_i8(f),get_i8(f), get_i8(f), get_i8(f))
                        face_uv = ((f_uv[0],f_uv[1],f_uv[2],f_uv[3],f_uv[6],f_uv[7],f_uv[4],f_uv[5]))
                        pad(f,2)
                        new = bm.faces.new(face_vertices)
                        color = [0,0,0,0]
                        color[0] = int32torgb(get_u32(f))
                        color[1] = int32torgb(get_u32(f))
                        color[3] = int32torgb(get_u32(f))
                        color[2] = int32torgb(get_u32(f))
                        for i, loop in enumerate(new.loops):
                            loop[collayer] = color[i]
                    #case 9: #LF2 - never seen
                    #case 10 | 11: #TSPR
                    case 10 | 11: #BSPR
                        pad(f, 12)
                    case 12: #LSF3
                        face_vertices = [bm.verts[get_i16(f)],bm.verts[get_i16(f)],bm.verts[get_i16(f)]]
                        pad(f, 2) #shall be a normal
                        new = bm.faces.new(face_vertices)
                        color = int32torgb(get_u32(f))
                        for loop in enumerate(new.loops):
                            loop[collayer] = color
                    case 13: #LSFT3
                        face_vertices = [bm.verts[get_i16(f)],bm.verts[get_i16(f)],bm.verts[get_i16(f)]]
                        pad(f, 2) #shall be a normal
                        texture = get_i16(f)
                        pad(f, 4)
                        face_uv = (get_i8(f),get_i8(f),get_i8(f),get_i8(f),get_i8(f),get_i8(f))
                        color = int32torgb(get_u32(f))
                        new = bm.faces.new(face_vertices)
                        for loop in new.loops:
                            loop[collayer] = color
                    case 14: #LSF4
                        point = (get_i16(f),get_i16(f),get_i16(f),get_i16(f))
                        face_vertices = [bm.verts[point[0]],bm.verts[point[1]],bm.verts[point[3]],bm.verts[point[2]]]
                        pad(f, 2) #shall be a normal
                        pad(f, 2)
                        color = int32torgb(get_u32(f))
                        new = bm.faces.new(face_vertices)
                        for loop in new.loops:
                            loop[collayer] = color
                    case 15: #LSFT4
                        point = (get_i16(f),get_i16(f),get_i16(f),get_i16(f))
                        face_vertices = [bm.verts[point[0]],bm.verts[point[1]],bm.verts[point[3]],bm.verts[point[2]]]
                        pad(f, 2) #shall be a normal
                        texture = get_i16(f)
                        pad(f, 4)
                        f_uv = (get_i8(f),get_i8(f),get_i8(f),get_i8(f),get_i8(f),get_i8(f), get_i8(f), get_i8(f))
                        face_uv = ((f_uv[0],f_uv[1],f_uv[2],f_uv[3],f_uv[6],f_uv[7],f_uv[4],f_uv[5]))
                        color = int32torgb(get_u32(f))
                        new = bm.faces.new(face_vertices)
                        for loop in new.loops:
                            loop[collayer] = color
                    case 16: #LSG3
                        face_vertices = [bm.verts[get_i16(f)],bm.verts[get_i16(f)],bm.verts[get_i16(f)]]
                        pad(f, 6) #shall be a normal
                        new = bm.faces.new(face_vertices)
                        color = [0,0,0]
                        color[0] = int32torgb(get_u32(f))
                        color[1] = int32torgb(get_u32(f))
                        color[2] = int32torgb(get_u32(f))
                        for i, loop in enumerate(new.loops):
                            loop[collayer] = color[i]
                    case 17: #LSGT3
                        face_vertices = [bm.verts[get_i16(f)],bm.verts[get_i16(f)],bm.verts[get_i16(f)]]
                        pad(f, 6) #shall be a normal
                        texture = get_i16(f)
                        pad(f, 4)
                        face_uv = (get_i8(f),get_i8(f),get_i8(f),get_i8(f),get_i8(f),get_i8(f))
                        new = bm.faces.new(face_vertices)
                        color = [0,0,0]
                        color[0] = int32torgb(get_u32(f))
                        color[1] = int32torgb(get_u32(f))
                        color[2] = int32torgb(get_u32(f))
                        for i, loop in enumerate(new.loops):
                            loop[collayer] = color[i]
                    case 18: #LSG4
                        point = (get_i16(f),get_i16(f),get_i16(f),get_i16(f))
                        face_vertices = [bm.verts[point[0]],bm.verts[point[1]],bm.verts[point[3]],bm.verts[point[2]]]
                        pad(f, 8) #shall be a normal
                        new = bm.faces.new(face_vertices)
                        color = [0,0,0,0]
                        color[0] = int32torgb(get_u32(f))
                        color[1] = int32torgb(get_u32(f))
                        color[3] = int32torgb(get_u32(f))
                        color[2] = int32torgb(get_u32(f))
                        for i, loop in enumerate(new.loops):
                            loop[collayer] = color[i]
                    case 19: #LSGT4
                        point = (get_i16(f),get_i16(f),get_i16(f),get_i16(f))
                        face_vertices = [bm.verts[point[0]],bm.verts[point[1]],bm.verts[point[3]],bm.verts[point[2]]]
                        pad(f, 8) #shall be a normal
                        texture = get_i16(f)
                        pad(f, 4)
                        face_uv = (get_i8(f),get_i8(f),get_i8(f),get_i8(f),get_i8(f),get_i8(f),get_i8(f),get_i8(f))
                        pad(f, 2)
                        new = bm.faces.new(face_vertices)
                        color = [0,0,0,0]
                        color[0] = int32torgb(get_u32(f))
                        color[1] = int32torgb(get_u32(f))
                        color[3] = int32torgb(get_u32(f))
                        color[2] = int32torgb(get_u32(f))
                        for i, loop in enumerate(new.loops):
                            loop[collayer] = color[i]
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
            bm.to_mesh(mesh_data)
            mesh_data.update()
            set_act = mesh_data.color_attributes.get("Attribute")
            mesh_data.attributes.active_color = set_act
            chunk = f.read(16)
            bm.free()
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
